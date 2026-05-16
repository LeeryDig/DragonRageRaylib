#include "levelLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "raymath.h"
#include "rlgl.h"
#include "utils.hpp"

namespace {

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
    explicit JsonParser(const std::string& textIn) : text(textIn), pos(0) {}

    JsonValue Parse() {
        SkipWhitespace();
        return ParseValue();
    }

  private:
    const std::string& text;
    std::size_t pos;

    void SkipWhitespace() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
    }

    bool Match(const char* token) {
        std::size_t len = std::char_traits<char>::length(token);
        if (text.compare(pos, len, token) == 0) {
            pos += len;
            return true;
        }
        return false;
    }

    JsonValue ParseValue() {
        SkipWhitespace();
        if (pos >= text.size()) {
            return JsonValue();
        }
        char c = text[pos];
        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == '"') return ParseString();
        if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber();
        JsonValue value;
        if (Match("true")) { value.type = JsonValue::Bool; value.boolValue = true; return value; }
        if (Match("false")) { value.type = JsonValue::Bool; value.boolValue = false; return value; }
        Match("null");
        return value;
    }

    JsonValue ParseObject() {
        JsonValue value;
        value.type = JsonValue::Object;
        ++pos;
        SkipWhitespace();
        if (pos < text.size() && text[pos] == '}') { ++pos; return value; }
        while (pos < text.size()) {
            JsonValue key = ParseString();
            SkipWhitespace();
            if (pos < text.size() && text[pos] == ':') ++pos;
            JsonValue child = ParseValue();
            value.objectValue[key.stringValue] = child;
            SkipWhitespace();
            if (pos < text.size() && text[pos] == ',') { ++pos; continue; }
            if (pos < text.size() && text[pos] == '}') { ++pos; break; }
        }
        return value;
    }

    JsonValue ParseArray() {
        JsonValue value;
        value.type = JsonValue::Array;
        ++pos;
        SkipWhitespace();
        if (pos < text.size() && text[pos] == ']') { ++pos; return value; }
        while (pos < text.size()) {
            value.arrayValue.push_back(ParseValue());
            SkipWhitespace();
            if (pos < text.size() && text[pos] == ',') { ++pos; continue; }
            if (pos < text.size() && text[pos] == ']') { ++pos; break; }
        }
        return value;
    }

    JsonValue ParseString() {
        JsonValue value;
        value.type = JsonValue::String;
        if (pos >= text.size() || text[pos] != '"') return value;
        ++pos;
        while (pos < text.size()) {
            char c = text[pos++];
            if (c == '"') break;
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
        return value;
    }

    JsonValue ParseNumber() {
        JsonValue value;
        value.type = JsonValue::Number;
        const char* start = text.c_str() + pos;
        char* end = nullptr;
        value.numberValue = std::strtod(start, &end);
        pos += static_cast<std::size_t>(end - start);
        return value;
    }
};

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

std::string StringMember(const JsonValue& value, const char* name, const std::string& fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::String ? member->stringValue : fallback;
}

Vector3 Vector3Member(const JsonValue& value, const char* name, Vector3 fallback) {
    const JsonValue* member = GetMember(value, name);
    return Vector3{NumberAt(member, 0, fallback.x), NumberAt(member, 1, fallback.y), NumberAt(member, 2, fallback.z)};
}

Quaternion QuaternionMember(const JsonValue& value, const char* name, Quaternion fallback) {
    const JsonValue* member = GetMember(value, name);
    return Quaternion{NumberAt(member, 0, fallback.x), NumberAt(member, 1, fallback.y), NumberAt(member, 2, fallback.z), NumberAt(member, 3, fallback.w)};
}

bool StartsWith(const std::string& text, const char* prefix) {
    std::string p(prefix);
    return text.size() >= p.size() && text.compare(0, p.size(), p) == 0;
}

LevelDebugNodeKind KindFromNodeName(const std::string& name) {
    if (StartsWith(name, "VISUAL_")) return LevelDebugNodeKind::Visual;
    if (StartsWith(name, "COL_")) return LevelDebugNodeKind::Collider;
    if (StartsWith(name, "ICON_")) return LevelDebugNodeKind::Icon;
    if (StartsWith(name, "SPAWN_")) return LevelDebugNodeKind::Spawn;
    if (StartsWith(name, "TRIGGER_")) return LevelDebugNodeKind::Trigger;
    if (StartsWith(name, "CHECKPOINT_") || name == "FINISH_LINE") return LevelDebugNodeKind::Trigger;
    return LevelDebugNodeKind::Other;
}

std::string DebugGroupName(const std::string& name) {
    std::size_t start = name.find('_');
    if (start == std::string::npos) return name;
    ++start;
    std::size_t end = name.find('_', start);
    return name.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

int GetOrCreateDebugGroup(LevelData& level, const std::string& name) {
    std::string groupName = DebugGroupName(name);
    for (std::size_t i = 0; i < level.debugGroups.size(); ++i) {
        if (level.debugGroups[i].name == groupName) return static_cast<int>(i);
    }
    LevelDebugGroup group;
    group.name = groupName;
    level.debugGroups.push_back(group);
    return static_cast<int>(level.debugGroups.size() - 1);
}

Matrix ComposeTransform(Vector3 translation, Quaternion rotation, Vector3 scale) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), QuaternionToMatrix(rotation)), MatrixTranslate(translation.x, translation.y, translation.z));
}

int CheckpointIndexFromName(const std::string& name) {
    std::size_t pos = name.find_last_of('_');
    if (pos == std::string::npos || pos + 1 >= name.size()) return 0;
    return std::atoi(name.c_str() + pos + 1);
}

bool ReadGlbChunks(const std::string& path, std::string& jsonText, std::vector<unsigned char>& binData) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) return false;

    std::uint32_t magic = 0, version = 0, length = 0;
    file.read(reinterpret_cast<char*>(&magic), 4);
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&length), 4);
    if (!file.good() || magic != 0x46546C67u || version != 2u) return false;

    while (file.good() && file.tellg() < static_cast<std::streampos>(length)) {
        std::uint32_t chunkLength = 0, chunkType = 0;
        file.read(reinterpret_cast<char*>(&chunkLength), 4);
        file.read(reinterpret_cast<char*>(&chunkType), 4);
        if (!file.good()) break;
        std::vector<unsigned char> chunk(chunkLength);
        file.read(reinterpret_cast<char*>(&chunk[0]), chunkLength);
        if (chunkType == 0x4E4F534Au) {
            jsonText.assign(reinterpret_cast<const char*>(&chunk[0]), chunk.size());
        } else if (chunkType == 0x004E4942u) {
            binData = chunk;
        }
    }
    return !jsonText.empty();
}

struct ParsedNode {
    std::string name;
    int mesh;
    int parent;
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
    bool hasMatrix;
    std::vector<float> matrix;
    std::vector<int> children;
};

struct MeshBounds {
    Vector3 center;
    Vector3 size;
};

struct MeshGeometry {
    MeshBounds bounds;
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
};

MeshBounds MeshBoundsFromAccessors(const JsonValue& root, const JsonValue& meshValue) {
    MeshBounds bounds;
    bounds.center = Vector3{0.0f, 0.0f, 0.0f};
    bounds.size = Vector3{1.0f, 1.0f, 1.0f};

    const JsonValue* primitives = GetMember(meshValue, "primitives");
    if (!primitives || primitives->type != JsonValue::Array || primitives->arrayValue.empty()) {
        return bounds;
    }
    const JsonValue* attributes = GetMember(primitives->arrayValue[0], "attributes");
    int positionAccessor = IntMember(attributes ? *attributes : JsonValue(), "POSITION", -1);
    const JsonValue* accessors = GetMember(root, "accessors");
    if (!accessors || accessors->type != JsonValue::Array || positionAccessor < 0 ||
        positionAccessor >= static_cast<int>(accessors->arrayValue.size())) {
        return bounds;
    }

    const JsonValue& accessor = accessors->arrayValue[positionAccessor];
    const JsonValue* minValue = GetMember(accessor, "min");
    const JsonValue* maxValue = GetMember(accessor, "max");
    Vector3 minPoint = Vector3{NumberAt(minValue, 0, -0.5f), NumberAt(minValue, 1, -0.5f), NumberAt(minValue, 2, -0.5f)};
    Vector3 maxPoint = Vector3{NumberAt(maxValue, 0, 0.5f), NumberAt(maxValue, 1, 0.5f), NumberAt(maxValue, 2, 0.5f)};
    bounds.center = Vector3Scale(Vector3Add(minPoint, maxPoint), 0.5f);
    bounds.size = Vector3Subtract(maxPoint, minPoint);
    return bounds;
}

template <typename T>
T ReadPod(const std::vector<unsigned char>& data, std::size_t offset) {
    T value;
    std::memcpy(&value, &data[offset], sizeof(T));
    return value;
}

int ComponentSize(int componentType) {
    switch (componentType) {
        case 5120:
        case 5121: return 1;
        case 5122:
        case 5123: return 2;
        case 5125:
        case 5126: return 4;
        default: return 0;
    }
}

int TypeComponentCount(const std::string& type) {
    if (type == "SCALAR") return 1;
    if (type == "VEC2") return 2;
    if (type == "VEC3") return 3;
    if (type == "VEC4") return 4;
    return 1;
}

std::size_t AccessorDataOffset(const JsonValue& root, int accessorIndex, int& componentType, std::string& type, int& count, int& stride) {
    componentType = 0;
    type = "";
    count = 0;
    stride = 0;
    const JsonValue* accessors = GetMember(root, "accessors");
    if (!accessors || accessors->type != JsonValue::Array || accessorIndex < 0 || accessorIndex >= static_cast<int>(accessors->arrayValue.size())) return 0;
    const JsonValue& accessor = accessors->arrayValue[accessorIndex];
    int bufferViewIndex = IntMember(accessor, "bufferView", -1);
    componentType = IntMember(accessor, "componentType", 0);
    type = StringMember(accessor, "type", "");
    count = IntMember(accessor, "count", 0);
    int accessorByteOffset = IntMember(accessor, "byteOffset", 0);

    const JsonValue* bufferViews = GetMember(root, "bufferViews");
    if (!bufferViews || bufferViews->type != JsonValue::Array || bufferViewIndex < 0 || bufferViewIndex >= static_cast<int>(bufferViews->arrayValue.size())) return 0;
    const JsonValue& bufferView = bufferViews->arrayValue[bufferViewIndex];
    int viewByteOffset = IntMember(bufferView, "byteOffset", 0);
    stride = IntMember(bufferView, "byteStride", ComponentSize(componentType) * TypeComponentCount(type));
    return static_cast<std::size_t>(viewByteOffset + accessorByteOffset);
}

std::vector<Vector3> ReadPositionAccessor(const JsonValue& root, const std::vector<unsigned char>& binData, int accessorIndex) {
    std::vector<Vector3> vertices;
    int componentType = 0, count = 0, stride = 0;
    std::string type;
    std::size_t offset = AccessorDataOffset(root, accessorIndex, componentType, type, count, stride);
    if (componentType != 5126 || type != "VEC3" || count <= 0 || stride <= 0) return vertices;
    vertices.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        std::size_t base = offset + static_cast<std::size_t>(i * stride);
        if (base + sizeof(float) * 3 > binData.size()) break;
        vertices.push_back(Vector3{
            ReadPod<float>(binData, base),
            ReadPod<float>(binData, base + sizeof(float)),
            ReadPod<float>(binData, base + sizeof(float) * 2)
        });
    }
    return vertices;
}

std::vector<unsigned int> ReadIndexAccessor(const JsonValue& root, const std::vector<unsigned char>& binData, int accessorIndex) {
    std::vector<unsigned int> indices;
    int componentType = 0, count = 0, stride = 0;
    std::string type;
    std::size_t offset = AccessorDataOffset(root, accessorIndex, componentType, type, count, stride);
    if (type != "SCALAR" || count <= 0 || stride <= 0) return indices;
    indices.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        std::size_t base = offset + static_cast<std::size_t>(i * stride);
        if (base >= binData.size()) break;
        if (componentType == 5121) {
            indices.push_back(static_cast<unsigned int>(binData[base]));
        } else if (componentType == 5123 && base + 2 <= binData.size()) {
            indices.push_back(static_cast<unsigned int>(ReadPod<std::uint16_t>(binData, base)));
        } else if (componentType == 5125 && base + 4 <= binData.size()) {
            indices.push_back(ReadPod<std::uint32_t>(binData, base));
        }
    }
    return indices;
}

bool IsRoadSurfaceName(const std::string& name) {
    return StartsWith(name, "COL_Road") || StartsWith(name, "COL_ROAD") || StartsWith(name, "COL_Ramp") || StartsWith(name, "COL_RAMP");
}

void ParseLevelMetadata(const std::string& path, LevelData& level) {
    std::string jsonText;
    std::vector<unsigned char> binData;
    if (!ReadGlbChunks(path, jsonText, binData)) {
        TraceLog(LOG_WARNING, "Level: could not read GLB chunks: %s", path.c_str());
        return;
    }

    JsonValue root = JsonParser(jsonText).Parse();
    const JsonValue* nodesValue = GetMember(root, "nodes");
    if (!nodesValue || nodesValue->type != JsonValue::Array) {
        TraceLog(LOG_WARNING, "Level: GLB has no nodes: %s", path.c_str());
        return;
    }

    std::vector<MeshGeometry> meshGeometries;
    const JsonValue* meshesValue = GetMember(root, "meshes");
    if (meshesValue && meshesValue->type == JsonValue::Array) {
        for (std::size_t i = 0; i < meshesValue->arrayValue.size(); ++i) {
            MeshGeometry geometry;
            geometry.bounds = MeshBoundsFromAccessors(root, meshesValue->arrayValue[i]);
            const JsonValue* primitives = GetMember(meshesValue->arrayValue[i], "primitives");
            if (primitives && primitives->type == JsonValue::Array) {
                for (std::size_t p = 0; p < primitives->arrayValue.size(); ++p) {
                    const JsonValue* attributes = GetMember(primitives->arrayValue[p], "attributes");
                    int positionAccessor = IntMember(attributes ? *attributes : JsonValue(), "POSITION", -1);
                    std::vector<Vector3> primitiveVertices = ReadPositionAccessor(root, binData, positionAccessor);
                    unsigned int vertexBase = static_cast<unsigned int>(geometry.vertices.size());
                    geometry.vertices.insert(geometry.vertices.end(), primitiveVertices.begin(), primitiveVertices.end());

                    int indexAccessor = IntMember(primitives->arrayValue[p], "indices", -1);
                    std::vector<unsigned int> primitiveIndices = ReadIndexAccessor(root, binData, indexAccessor);
                    if (!primitiveIndices.empty()) {
                        for (std::size_t index = 0; index < primitiveIndices.size(); ++index) {
                            geometry.indices.push_back(vertexBase + primitiveIndices[index]);
                        }
                    } else {
                        for (std::size_t index = 0; index < primitiveVertices.size(); ++index) {
                            geometry.indices.push_back(vertexBase + static_cast<unsigned int>(index));
                        }
                    }
                }
            }
            meshGeometries.push_back(geometry);
        }
    }

    std::vector<ParsedNode> nodes;
    for (std::size_t i = 0; i < nodesValue->arrayValue.size(); ++i) {
        const JsonValue& nodeValue = nodesValue->arrayValue[i];
        ParsedNode node;
        node.name = StringMember(nodeValue, "name", "");
        node.mesh = IntMember(nodeValue, "mesh", -1);
        node.parent = -1;
        node.translation = Vector3Member(nodeValue, "translation", Vector3{0.0f, 0.0f, 0.0f});
        node.rotation = QuaternionMember(nodeValue, "rotation", Quaternion{0.0f, 0.0f, 0.0f, 1.0f});
        node.scale = Vector3Member(nodeValue, "scale", Vector3{1.0f, 1.0f, 1.0f});
        node.hasMatrix = false;
        const JsonValue* matrix = GetMember(nodeValue, "matrix");
        if (matrix && matrix->type == JsonValue::Array && matrix->arrayValue.size() >= 16) {
            node.hasMatrix = true;
            for (std::size_t m = 0; m < 16; ++m) {
                node.matrix.push_back(NumberAt(matrix, m, m % 5 == 0 ? 1.0f : 0.0f));
            }
            node.translation = Vector3{node.matrix[12], node.matrix[13], node.matrix[14]};
            Vector3 column0 = Vector3{node.matrix[0], node.matrix[1], node.matrix[2]};
            Vector3 column1 = Vector3{node.matrix[4], node.matrix[5], node.matrix[6]};
            Vector3 column2 = Vector3{node.matrix[8], node.matrix[9], node.matrix[10]};
            node.scale = Vector3{Vector3Length(column0), Vector3Length(column1), Vector3Length(column2)};
            if (node.scale.x > 0.0001f) column0 = Vector3Scale(column0, 1.0f / node.scale.x);
            if (node.scale.y > 0.0001f) column1 = Vector3Scale(column1, 1.0f / node.scale.y);
            if (node.scale.z > 0.0001f) column2 = Vector3Scale(column2, 1.0f / node.scale.z);
            Matrix rotationMatrix = Matrix{
                column0.x, column1.x, column2.x, 0.0f,
                column0.y, column1.y, column2.y, 0.0f,
                column0.z, column1.z, column2.z, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            node.rotation = QuaternionFromMatrix(rotationMatrix);
        }
        const JsonValue* children = GetMember(nodeValue, "children");
        if (children && children->type == JsonValue::Array) {
            for (std::size_t c = 0; c < children->arrayValue.size(); ++c) {
                if (children->arrayValue[c].type == JsonValue::Number) {
                    node.children.push_back(static_cast<int>(children->arrayValue[c].numberValue));
                }
            }
        }
        nodes.push_back(node);
    }

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        for (std::size_t c = 0; c < nodes[i].children.size(); ++c) {
            int child = nodes[i].children[c];
            if (child >= 0 && child < static_cast<int>(nodes.size())) {
                nodes[child].parent = static_cast<int>(i);
            }
        }
    }

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const ParsedNode& node = nodes[i];
        if (node.name.empty()) continue;

        Vector3 worldPosition = node.translation;
        Quaternion worldRotation = node.rotation;
        Vector3 worldScale = node.scale;
        int parent = node.parent;
        while (parent >= 0 && parent < static_cast<int>(nodes.size())) {
            const ParsedNode& p = nodes[parent];
            worldPosition = Vector3{worldPosition.x * p.scale.x, worldPosition.y * p.scale.y, worldPosition.z * p.scale.z};
            worldPosition = Vector3Add(p.translation, Vector3RotateByQuaternion(worldPosition, p.rotation));
            worldRotation = QuaternionNormalize(QuaternionMultiply(p.rotation, worldRotation));
            worldScale = Vector3{worldScale.x * p.scale.x, worldScale.y * p.scale.y, worldScale.z * p.scale.z};
            parent = p.parent;
        }

        MeshGeometry geometry;
        geometry.bounds.center = Vector3{0.0f, 0.0f, 0.0f};
        geometry.bounds.size = Vector3{1.0f, 1.0f, 1.0f};
        if (node.mesh >= 0 && node.mesh < static_cast<int>(meshGeometries.size())) {
            geometry = meshGeometries[node.mesh];
        }
        Vector3 scaledLocalCenter = Vector3{geometry.bounds.center.x * worldScale.x, geometry.bounds.center.y * worldScale.y, geometry.bounds.center.z * worldScale.z};
        Vector3 volumePosition = Vector3Add(worldPosition, Vector3RotateByQuaternion(scaledLocalCenter, worldRotation));
        Vector3 size = Vector3{fabsf(geometry.bounds.size.x * worldScale.x), fabsf(geometry.bounds.size.y * worldScale.y), fabsf(geometry.bounds.size.z * worldScale.z)};

        LevelDebugNode debugNode;
        debugNode.name = node.name;
        debugNode.kind = KindFromNodeName(node.name);
        debugNode.meshIndex = node.mesh;
        debugNode.groupId = GetOrCreateDebugGroup(level, node.name);
        debugNode.runtimeIndex = -1;
        debugNode.position = volumePosition;
        debugNode.rotation = worldRotation;
        debugNode.scale = worldScale;
        debugNode.size = size;

        if (StartsWith(node.name, "VISUAL_")) {
            level.renderParts.push_back(LevelRenderPart{node.mesh, ComposeTransform(worldPosition, worldRotation, worldScale)});
            debugNode.runtimeIndex = static_cast<int>(level.renderParts.size() - 1);
            level.debugNodes.push_back(debugNode);
            continue;
        }

        if (StartsWith(node.name, "ICON_")) {
            level.debugNodes.push_back(debugNode);
            continue;
        }

        if (StartsWith(node.name, "COL_")) {
            if (IsRoadSurfaceName(node.name)) {
                LevelRoadSurface surface;
                surface.name = node.name;
                surface.position = volumePosition;
                surface.rotation = worldRotation;
                surface.size = size;
                surface.indices = geometry.indices;
                surface.vertices.reserve(geometry.vertices.size());
                for (std::size_t vertex = 0; vertex < geometry.vertices.size(); ++vertex) {
                    Vector3 scaledVertex = Vector3{
                        geometry.vertices[vertex].x * worldScale.x,
                        geometry.vertices[vertex].y * worldScale.y,
                        geometry.vertices[vertex].z * worldScale.z
                    };
                    surface.vertices.push_back(Vector3Add(worldPosition, Vector3RotateByQuaternion(scaledVertex, worldRotation)));
                }
                level.roadSurfaces.push_back(surface);
                debugNode.runtimeIndex = static_cast<int>(level.roadSurfaces.size() - 1);
                TraceLog(LOG_INFO, "Level road surface: %s pos=(%.2f %.2f %.2f) size=(%.2f %.2f %.2f)",
                    surface.name.c_str(), surface.position.x, surface.position.y, surface.position.z,
                    surface.size.x, surface.size.y, surface.size.z);
            } else {
                LevelBoxVolume collider;
                collider.name = node.name;
                collider.position = volumePosition;
                collider.rotation = worldRotation;
                collider.size = size;
                level.colliders.push_back(collider);
                debugNode.runtimeIndex = static_cast<int>(level.colliders.size() - 1);
                TraceLog(LOG_INFO, "Level box collider: %s pos=(%.2f %.2f %.2f) size=(%.2f %.2f %.2f)",
                    collider.name.c_str(), collider.position.x, collider.position.y, collider.position.z,
                    collider.size.x, collider.size.y, collider.size.z);
            }
            level.debugNodes.push_back(debugNode);
        } else if (node.name == "SPAWN_PLAYER") {
            level.playerSpawn.position = worldPosition;
            level.playerSpawn.rotation = worldRotation;
            level.playerSpawn.valid = true;
            level.debugNodes.push_back(debugNode);
            TraceLog(LOG_INFO, "Level spawn: %s pos=(%.2f %.2f %.2f)", node.name.c_str(), worldPosition.x, worldPosition.y, worldPosition.z);
        } else if (StartsWith(node.name, "CHECKPOINT_")) {
            LevelCheckpoint checkpoint;
            checkpoint.name = node.name;
            checkpoint.index = CheckpointIndexFromName(node.name);
            checkpoint.position = worldPosition;
            checkpoint.rotation = worldRotation;
            checkpoint.size = size;
            level.checkpoints.push_back(checkpoint);
            debugNode.runtimeIndex = static_cast<int>(level.checkpoints.size() - 1);
            level.debugNodes.push_back(debugNode);
        } else if (node.name == "FINISH_LINE") {
            level.finishLine.name = node.name;
            level.finishLine.position = worldPosition;
            level.finishLine.rotation = worldRotation;
            level.finishLine.size = size;
            level.hasFinishLine = true;
            level.debugNodes.push_back(debugNode);
        } else if (node.mesh >= 0 || debugNode.kind != LevelDebugNodeKind::Other) {
            level.debugNodes.push_back(debugNode);
        }
    }

    std::sort(level.checkpoints.begin(), level.checkpoints.end(), [](const LevelCheckpoint& a, const LevelCheckpoint& b) {
        return a.index < b.index;
    });

    level.originalDebugNodes = level.debugNodes;
    level.originalColliders = level.colliders;
    level.originalRoadSurfaces = level.roadSurfaces;
    level.originalCheckpoints = level.checkpoints;
    level.originalPlayerSpawn = level.playerSpawn;
    level.originalFinishLine = level.finishLine;
}

}  // namespace

LevelData LoadLevel(const std::string& levelPath) {
    LevelData level;
    level.path = Utils::ResolveProjectPath(levelPath);
    level.name = levelPath;
    level.visualModel = LoadModel(level.path.c_str());
    level.visualLoaded = level.visualModel.meshCount > 0;
    if (!level.visualLoaded) {
        TraceLog(LOG_WARNING, "Level: failed to load visual model: %s", level.path.c_str());
    }
    ParseLevelMetadata(level.path, level);
    return level;
}

void UnloadLevel(LevelData& level) {
    if (level.visualLoaded) {
        UnloadModel(level.visualModel);
        level.visualLoaded = false;
    }
}

void DrawLevel(const LevelData& level) {
    if (level.visualLoaded) {
        Vector3 axis = Vector3{0.0f, 1.0f, 0.0f};
        float angle = 0.0f;
        QuaternionToAxisAngle(level.rootRotation, &axis, &angle);
        DrawModelEx(level.visualModel, level.rootPosition, axis, angle * RAD2DEG, Vector3{1.0f, 1.0f, 1.0f}, WHITE);
    }
}

const char* LevelDebugNodeKindName(LevelDebugNodeKind kind) {
    switch (kind) {
        case LevelDebugNodeKind::Visual: return "VISUAL";
        case LevelDebugNodeKind::Collider: return "COL";
        case LevelDebugNodeKind::Icon: return "ICON";
        case LevelDebugNodeKind::Spawn: return "SPAWN";
        case LevelDebugNodeKind::Trigger: return "TRIGGER";
        case LevelDebugNodeKind::Invisible: return "INVISIBLE";
        default: return "OTHER";
    }
}

Vector3 ApplyRootPoint(const LevelData& level, Vector3 point) {
    return Vector3Add(level.rootPosition, Vector3RotateByQuaternion(point, level.rootRotation));
}

Quaternion ApplyRootRotation(const LevelData& level, Quaternion rotation) {
    return QuaternionNormalize(QuaternionMultiply(level.rootRotation, rotation));
}

void ApplyLevelRootTransform(LevelData& level, Vector3 position, Quaternion rotation) {
    level.rootPosition = position;
    level.rootRotation = rotation;

    level.debugNodes = level.originalDebugNodes;
    for (std::size_t i = 0; i < level.debugNodes.size(); ++i) {
        level.debugNodes[i].position = ApplyRootPoint(level, level.originalDebugNodes[i].position);
        level.debugNodes[i].rotation = ApplyRootRotation(level, level.originalDebugNodes[i].rotation);
    }

    level.colliders = level.originalColliders;
    for (std::size_t i = 0; i < level.colliders.size(); ++i) {
        level.colliders[i].position = ApplyRootPoint(level, level.originalColliders[i].position);
        level.colliders[i].rotation = ApplyRootRotation(level, level.originalColliders[i].rotation);
    }

    level.roadSurfaces = level.originalRoadSurfaces;
    for (std::size_t i = 0; i < level.roadSurfaces.size(); ++i) {
        level.roadSurfaces[i].position = ApplyRootPoint(level, level.originalRoadSurfaces[i].position);
        level.roadSurfaces[i].rotation = ApplyRootRotation(level, level.originalRoadSurfaces[i].rotation);
        for (std::size_t v = 0; v < level.roadSurfaces[i].vertices.size(); ++v) {
            level.roadSurfaces[i].vertices[v] = ApplyRootPoint(level, level.originalRoadSurfaces[i].vertices[v]);
        }
    }

    level.checkpoints = level.originalCheckpoints;
    for (std::size_t i = 0; i < level.checkpoints.size(); ++i) {
        level.checkpoints[i].position = ApplyRootPoint(level, level.originalCheckpoints[i].position);
        level.checkpoints[i].rotation = ApplyRootRotation(level, level.originalCheckpoints[i].rotation);
    }

    level.playerSpawn = level.originalPlayerSpawn;
    if (level.playerSpawn.valid) {
        level.playerSpawn.position = ApplyRootPoint(level, level.originalPlayerSpawn.position);
        level.playerSpawn.rotation = ApplyRootRotation(level, level.originalPlayerSpawn.rotation);
    }

    if (level.hasFinishLine) {
        level.finishLine = level.originalFinishLine;
        level.finishLine.position = ApplyRootPoint(level, level.originalFinishLine.position);
        level.finishLine.rotation = ApplyRootRotation(level, level.originalFinishLine.rotation);
    }
}

void ApplyLevelDebugNodeTransform(LevelData& level, int debugNodeIndex, Vector3 position, Quaternion rotation, Vector3 scale) {
    if (debugNodeIndex < 0 || debugNodeIndex >= static_cast<int>(level.debugNodes.size())) return;
    LevelDebugNode& node = level.debugNodes[debugNodeIndex];
    if (node.kind == LevelDebugNodeKind::Visual) {
        return;
    }

    Vector3 delta = Vector3Subtract(position, node.position);
    Vector3 oldScale = node.scale;
    Vector3 scaleRatio = Vector3{
        fabsf(oldScale.x) > 0.0001f ? scale.x / oldScale.x : 1.0f,
        fabsf(oldScale.y) > 0.0001f ? scale.y / oldScale.y : 1.0f,
        fabsf(oldScale.z) > 0.0001f ? scale.z / oldScale.z : 1.0f
    };
    node.position = position;
    node.rotation = rotation;
    node.scale = scale;
    node.size = Vector3{node.size.x * scaleRatio.x, node.size.y * scaleRatio.y, node.size.z * scaleRatio.z};

    if (node.kind == LevelDebugNodeKind::Visual && node.groupId >= 0) {
        for (std::size_t i = 0; i < level.debugNodes.size(); ++i) {
            if (static_cast<int>(i) == debugNodeIndex) continue;
            if (level.debugNodes[i].groupId == node.groupId) {
                level.debugNodes[i].position = Vector3Add(level.debugNodes[i].position, delta);
                level.debugNodes[i].rotation = rotation;
                level.debugNodes[i].scale = Vector3{level.debugNodes[i].scale.x * scaleRatio.x, level.debugNodes[i].scale.y * scaleRatio.y, level.debugNodes[i].scale.z * scaleRatio.z};
                level.debugNodes[i].size = Vector3{level.debugNodes[i].size.x * scaleRatio.x, level.debugNodes[i].size.y * scaleRatio.y, level.debugNodes[i].size.z * scaleRatio.z};
            }
        }
    }

    for (std::size_t i = 0; i < level.debugNodes.size(); ++i) {
        const LevelDebugNode& n = level.debugNodes[i];
        if (n.kind == LevelDebugNodeKind::Visual && n.runtimeIndex >= 0 && n.runtimeIndex < static_cast<int>(level.renderParts.size())) {
            level.renderParts[n.runtimeIndex].transform = ComposeTransform(n.position, n.rotation, n.scale);
        } else if (n.kind == LevelDebugNodeKind::Collider && n.runtimeIndex >= 0) {
            if (IsRoadSurfaceName(n.name) && n.runtimeIndex < static_cast<int>(level.roadSurfaces.size())) {
                Vector3 roadDelta = Vector3Subtract(n.position, level.roadSurfaces[n.runtimeIndex].position);
                for (std::size_t v = 0; v < level.roadSurfaces[n.runtimeIndex].vertices.size(); ++v) {
                    level.roadSurfaces[n.runtimeIndex].vertices[v] = Vector3Add(level.roadSurfaces[n.runtimeIndex].vertices[v], roadDelta);
                }
                level.roadSurfaces[n.runtimeIndex].position = n.position;
                level.roadSurfaces[n.runtimeIndex].rotation = n.rotation;
                level.roadSurfaces[n.runtimeIndex].size = n.size;
            } else if (n.runtimeIndex < static_cast<int>(level.colliders.size())) {
                level.colliders[n.runtimeIndex].position = n.position;
                level.colliders[n.runtimeIndex].rotation = n.rotation;
                level.colliders[n.runtimeIndex].size = n.size;
            }
        } else if (n.kind == LevelDebugNodeKind::Spawn && n.name == "SPAWN_PLAYER") {
            level.playerSpawn.position = n.position;
            level.playerSpawn.rotation = n.rotation;
        } else if (n.kind == LevelDebugNodeKind::Trigger && n.runtimeIndex >= 0 && n.runtimeIndex < static_cast<int>(level.checkpoints.size())) {
            level.checkpoints[n.runtimeIndex].position = n.position;
            level.checkpoints[n.runtimeIndex].rotation = n.rotation;
            level.checkpoints[n.runtimeIndex].size = n.size;
        } else if (n.name == "FINISH_LINE") {
            level.finishLine.position = n.position;
            level.finishLine.rotation = n.rotation;
            level.finishLine.size = n.size;
        }
    }
}

void TeleportLevelDebugNodeToCamera(LevelData& level, int debugNodeIndex, const Camera& camera) {
    if (debugNodeIndex < 0 || debugNodeIndex >= static_cast<int>(level.debugNodes.size())) return;
    LevelDebugNode node = level.debugNodes[debugNodeIndex];
    ApplyLevelDebugNodeTransform(level, debugNodeIndex, camera.position, node.rotation, node.scale);
}

void DrawLevelDebugSelection(const LevelData& level, int debugNodeIndex, const Camera& camera) {
    if (debugNodeIndex < 0 || debugNodeIndex >= static_cast<int>(level.debugNodes.size())) return;
    const LevelDebugNode& node = level.debugNodes[debugNodeIndex];
    DrawLine3D(node.position, Vector3Add(node.position, Vector3{1.0f, 0.0f, 0.0f}), RED);
    DrawLine3D(node.position, Vector3Add(node.position, Vector3{0.0f, 1.0f, 0.0f}), GREEN);
    DrawLine3D(node.position, Vector3Add(node.position, Vector3{0.0f, 0.0f, 1.0f}), BLUE);

    if (node.kind == LevelDebugNodeKind::Collider) {
        rlPushMatrix();
        rlTranslatef(node.position.x, node.position.y, node.position.z);
        Vector3 axis = Vector3{0.0f, 1.0f, 0.0f};
        float angle = 0.0f;
        QuaternionToAxisAngle(node.rotation, &axis, &angle);
        rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);
        DrawCubeWiresV(Vector3Zero(), node.size, ORANGE);
        rlPopMatrix();
    }

    if (node.kind == LevelDebugNodeKind::Spawn || node.kind == LevelDebugNodeKind::Trigger || node.kind == LevelDebugNodeKind::Icon) {
        Vector3 up = camera.up;
        Vector3 right = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(camera.target, camera.position), up));
        float size = 0.35f;
        Vector3 a = Vector3Add(node.position, Vector3Scale(up, size));
        Vector3 b = Vector3Add(node.position, Vector3Scale(right, size));
        Vector3 c = Vector3Subtract(node.position, Vector3Scale(up, size));
        Vector3 d = Vector3Subtract(node.position, Vector3Scale(right, size));
        Color color = node.kind == LevelDebugNodeKind::Spawn ? GREEN : SKYBLUE;
        DrawTriangle3D(a, b, c, color);
        DrawTriangle3D(a, c, d, color);
    }
}
