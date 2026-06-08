#ifndef ASSETS_JSON_HPP
#define ASSETS_JSON_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace assets {

struct JsonValue {
    enum Type { Null, Bool, Number, String, Array, Object } type;
    bool boolValue;
    double numberValue;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    JsonValue() : type(Null), boolValue(false), numberValue(0.0) {}
};

class JsonParser {
  public:
    explicit JsonParser(const std::string& textIn);
    JsonValue Parse();
    bool HadError() const;
    const std::string& Error() const;

  private:
    const std::string& text;
    std::size_t pos;
    bool hadError;
    std::string error;

    void SkipWhitespace();
    bool Match(const char* token);
    void SetError(const char* message);
    JsonValue ParseValue();
    JsonValue ParseObject();
    JsonValue ParseArray();
    JsonValue ParseString();
    JsonValue ParseNumber();
};

const JsonValue* GetMember(const JsonValue& value, const char* name);
float NumberAt(const JsonValue* arrayValue, std::size_t index, float fallback);
int IntMember(const JsonValue& value, const char* name, int fallback);
float FloatMember(const JsonValue& value, const char* name, float fallback);
bool BoolMember(const JsonValue& value, const char* name, bool fallback);
std::string StringMember(const JsonValue& value, const char* name, const std::string& fallback);

}  // namespace assets

#endif
