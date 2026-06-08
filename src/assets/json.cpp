#include "json.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace assets {

JsonParser::JsonParser(const std::string& textIn)
    : text(textIn), pos(0), hadError(false), error() {}

JsonValue JsonParser::Parse() {
    SkipWhitespace();
    JsonValue value = ParseValue();
    SkipWhitespace();
    if (pos < text.size()) SetError("unexpected trailing JSON content");
    return value;
}

bool JsonParser::HadError() const { return hadError; }
const std::string& JsonParser::Error() const { return error; }

void JsonParser::SkipWhitespace() {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
}

bool JsonParser::Match(const char* token) {
    std::size_t len = std::strlen(token);
    if (text.compare(pos, len, token) == 0) {
        pos += len;
        return true;
    }
    return false;
}

void JsonParser::SetError(const char* message) {
    if (hadError) return;
    hadError = true;
    error = message;
    error += " at byte ";
    error += std::to_string(pos);
}

JsonValue JsonParser::ParseValue() {
    SkipWhitespace();
    if (pos >= text.size()) { SetError("unexpected end of JSON"); return JsonValue(); }
    char c = text[pos];
    if (c == '{') return ParseObject();
    if (c == '[') return ParseArray();
    if (c == '"') return ParseString();
    if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber();
    JsonValue value;
    if (Match("true")) { value.type = JsonValue::Bool; value.boolValue = true; return value; }
    if (Match("false")) { value.type = JsonValue::Bool; value.boolValue = false; return value; }
    if (Match("null")) return value;
    SetError("invalid JSON value");
    return value;
}

JsonValue JsonParser::ParseObject() {
    JsonValue value;
    value.type = JsonValue::Object;
    ++pos;
    SkipWhitespace();
    if (pos < text.size() && text[pos] == '}') { ++pos; return value; }
    while (pos < text.size()) {
        if (text[pos] != '"') { SetError("expected object key string"); return value; }
        JsonValue key = ParseString();
        SkipWhitespace();
        if (pos >= text.size() || text[pos] != ':') { SetError("expected ':' after object key"); return value; }
        ++pos;
        value.objectValue[key.stringValue] = ParseValue();
        SkipWhitespace();
        if (pos < text.size() && text[pos] == ',') { ++pos; SkipWhitespace(); continue; }
        if (pos < text.size() && text[pos] == '}') { ++pos; return value; }
        SetError("expected ',' or '}' in object");
        return value;
    }
    SetError("unterminated object");
    return value;
}

JsonValue JsonParser::ParseArray() {
    JsonValue value;
    value.type = JsonValue::Array;
    ++pos;
    SkipWhitespace();
    if (pos < text.size() && text[pos] == ']') { ++pos; return value; }
    while (pos < text.size()) {
        value.arrayValue.push_back(ParseValue());
        SkipWhitespace();
        if (pos < text.size() && text[pos] == ',') { ++pos; SkipWhitespace(); continue; }
        if (pos < text.size() && text[pos] == ']') { ++pos; return value; }
        SetError("expected ',' or ']' in array");
        return value;
    }
    SetError("unterminated array");
    return value;
}

JsonValue JsonParser::ParseString() {
    JsonValue value;
    value.type = JsonValue::String;
    if (pos >= text.size() || text[pos] != '"') { SetError("expected string"); return value; }
    ++pos;
    while (pos < text.size()) {
        char c = text[pos++];
        if (c == '"') return value;
        if (c == '\\' && pos < text.size()) {
            char escaped = text[pos++];
            switch (escaped) {
                case '"': value.stringValue.push_back('"'); break;
                case '\\': value.stringValue.push_back('\\'); break;
                case '/': value.stringValue.push_back('/'); break;
                case 'b': value.stringValue.push_back('\b'); break;
                case 'f': value.stringValue.push_back('\f'); break;
                case 'n': value.stringValue.push_back('\n'); break;
                case 'r': value.stringValue.push_back('\r'); break;
                case 't': value.stringValue.push_back('\t'); break;
                default: value.stringValue.push_back(escaped); break;
            }
        } else {
            value.stringValue.push_back(c);
        }
    }
    SetError("unterminated string");
    return value;
}

JsonValue JsonParser::ParseNumber() {
    JsonValue value;
    value.type = JsonValue::Number;
    const char* start = text.c_str() + pos;
    char* end = nullptr;
    value.numberValue = std::strtod(start, &end);
    if (end == start) { SetError("invalid number"); return value; }
    pos += static_cast<std::size_t>(end - start);
    return value;
}

const JsonValue* GetMember(const JsonValue& value, const char* name) {
    if (value.type != JsonValue::Object) return nullptr;
    std::map<std::string, JsonValue>::const_iterator it = value.objectValue.find(name);
    return it == value.objectValue.end() ? nullptr : &it->second;
}

float NumberAt(const JsonValue* arrayValue, std::size_t index, float fallback) {
    if (!arrayValue || arrayValue->type != JsonValue::Array || index >= arrayValue->arrayValue.size()) return fallback;
    const JsonValue& value = arrayValue->arrayValue[index];
    return value.type == JsonValue::Number ? static_cast<float>(value.numberValue) : fallback;
}

int IntMember(const JsonValue& value, const char* name, int fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::Number ? static_cast<int>(member->numberValue) : fallback;
}

float FloatMember(const JsonValue& value, const char* name, float fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::Number ? static_cast<float>(member->numberValue) : fallback;
}

bool BoolMember(const JsonValue& value, const char* name, bool fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::Bool ? member->boolValue : fallback;
}

std::string StringMember(const JsonValue& value, const char* name, const std::string& fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::String ? member->stringValue : fallback;
}

}  // namespace assets
