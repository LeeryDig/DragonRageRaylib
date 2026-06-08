#include "interactionSystem.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "raylib.h"
#include "raymath.h"
#include "assets/json.hpp"
#include "utils.hpp"

namespace {

// ─── JSON helpers ─────────────────────────────────────────────────────────────

using assets::GetMember;
using assets::JsonParser;
using assets::JsonValue;
using assets::NumberAt;
using assets::StringMember;

std::vector<InteractionChoice> ExtractChoices(const JsonValue& characterConfig) {
    std::vector<InteractionChoice> choices;
    const JsonValue* choicesValue = GetMember(characterConfig, "choices");
    if (!choicesValue || choicesValue->type != JsonValue::Array) return choices;
    for (std::size_t i = 0; i < choicesValue->arrayValue.size(); ++i) {
        const JsonValue& item = choicesValue->arrayValue[i];
        if (item.type != JsonValue::Object) continue;
        InteractionChoice choice;
        choice.text = StringMember(item, "text", "...");
        choices.push_back(choice);
    }
    return choices;
}

// ─── GLB/GLTF parser ──────────────────────────────────────────────────────────

bool StartsWith(const std::string& text, const char* prefix) {
    std::string p(prefix);
    return text.size() >= p.size() && text.compare(0, p.size(), p) == 0;
}

using assets::GetMember;
using assets::IntMember;
using assets::JsonParser;
using assets::JsonValue;
using assets::NumberAt;
using assets::StringMember;

Vector3 Vector3Member(const JsonValue& value, const char* name, Vector3 fallback) {
    const JsonValue* member = GetMember(value, name);
    return Vector3{NumberAt(member, 0, fallback.x), NumberAt(member, 1, fallback.y), NumberAt(member, 2, fallback.z)};
}

Quaternion QuaternionMember(const JsonValue& value, const char* name, Quaternion fallback) {
    const JsonValue* member = GetMember(value, name);
    return Quaternion{
        NumberAt(member, 0, fallback.x), NumberAt(member, 1, fallback.y),
        NumberAt(member, 2, fallback.z), NumberAt(member, 3, fallback.w)};
}

bool ReadGlbJson(const std::string& path, std::string& jsonText) {
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
            return true;
        }
    }
    return false;
}

struct ParsedNode {
    std::string name;
    int mesh;
    int parent;
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
    std::vector<float> matrix;
    std::vector<int> children;
};

struct MeshBounds {
    Vector3 center;
    Vector3 size;
};

MeshBounds MeshBoundsFromAccessors(const JsonValue& root, const JsonValue& meshValue) {
    MeshBounds bounds;
    bounds.center = Vector3{0.0f, 0.0f, 0.0f};
    bounds.size = Vector3{1.0f, 1.0f, 1.0f};
    const JsonValue* primitives = GetMember(meshValue, "primitives");
    if (!primitives || primitives->type != JsonValue::Array || primitives->arrayValue.empty()) return bounds;
    const JsonValue* attributes = GetMember(primitives->arrayValue[0], "attributes");
    int positionAccessor = IntMember(attributes ? *attributes : JsonValue(), "POSITION", -1);
    const JsonValue* accessors = GetMember(root, "accessors");
    if (!accessors || accessors->type != JsonValue::Array || positionAccessor < 0
        || positionAccessor >= static_cast<int>(accessors->arrayValue.size())) return bounds;
    const JsonValue& accessor = accessors->arrayValue[positionAccessor];
    const JsonValue* minValue = GetMember(accessor, "min");
    const JsonValue* maxValue = GetMember(accessor, "max");
    Vector3 minPoint = Vector3{NumberAt(minValue, 0, -0.5f), NumberAt(minValue, 1, -0.5f), NumberAt(minValue, 2, -0.5f)};
    Vector3 maxPoint = Vector3{NumberAt(maxValue, 0, 0.5f), NumberAt(maxValue, 1, 0.5f), NumberAt(maxValue, 2, 0.5f)};
    bounds.center = Vector3Scale(Vector3Add(minPoint, maxPoint), 0.5f);
    bounds.size = Vector3Subtract(maxPoint, minPoint);
    return bounds;
}

Matrix ComposeTransform(Vector3 translation, Quaternion rotation, Vector3 scale) {
    return MatrixMultiply(
        MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), QuaternionToMatrix(rotation)),
        MatrixTranslate(translation.x, translation.y, translation.z));
}

void ParseCharacterModelMetadata(InteractableCharacter& character) {
    std::string jsonText;
    if (!ReadGlbJson(character.modelPath, jsonText)) return;
    JsonParser parser(jsonText);
    JsonValue root = parser.Parse();
    if (parser.HadError()) {
        TraceLog(LOG_WARNING, "Character metadata: GLB JSON parse warning in %s: %s", character.modelPath.c_str(), parser.Error().c_str());
    }

    std::vector<MeshBounds> meshBounds;
    const JsonValue* meshesValue = GetMember(root, "meshes");
    if (meshesValue && meshesValue->type == JsonValue::Array) {
        for (std::size_t i = 0; i < meshesValue->arrayValue.size(); ++i) {
            meshBounds.push_back(MeshBoundsFromAccessors(root, meshesValue->arrayValue[i]));
        }
    }

    std::vector<ParsedNode> nodes;
    const JsonValue* nodesValue = GetMember(root, "nodes");
    if (!nodesValue || nodesValue->type != JsonValue::Array) return;
    for (std::size_t i = 0; i < nodesValue->arrayValue.size(); ++i) {
        const JsonValue& nodeValue = nodesValue->arrayValue[i];
        ParsedNode node;
        node.name = StringMember(nodeValue, "name", "");
        node.mesh = IntMember(nodeValue, "mesh", -1);
        node.parent = -1;
        node.translation = Vector3Member(nodeValue, "translation", Vector3{0.0f, 0.0f, 0.0f});
        node.rotation = QuaternionMember(nodeValue, "rotation", Quaternion{0.0f, 0.0f, 0.0f, 1.0f});
        node.scale = Vector3Member(nodeValue, "scale", Vector3{1.0f, 1.0f, 1.0f});
        const JsonValue* matrix = GetMember(nodeValue, "matrix");
        if (matrix && matrix->type == JsonValue::Array && matrix->arrayValue.size() >= 16) {
            for (std::size_t m = 0; m < 16; ++m)
                node.matrix.push_back(NumberAt(matrix, m, m % 5 == 0 ? 1.0f : 0.0f));
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
                0.0f, 0.0f, 0.0f, 1.0f};
            node.rotation = QuaternionFromMatrix(rotationMatrix);
        }
        const JsonValue* children = GetMember(nodeValue, "children");
        if (children && children->type == JsonValue::Array) {
            for (std::size_t c = 0; c < children->arrayValue.size(); ++c) {
                if (children->arrayValue[c].type == JsonValue::Number)
                    node.children.push_back(static_cast<int>(children->arrayValue[c].numberValue));
            }
        }
        nodes.push_back(node);
    }

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        for (std::size_t c = 0; c < nodes[i].children.size(); ++c) {
            int child = nodes[i].children[c];
            if (child >= 0 && child < static_cast<int>(nodes.size())) nodes[child].parent = static_cast<int>(i);
        }
    }

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const ParsedNode& node = nodes[i];
        if (node.name.empty() || node.mesh < 0 || node.mesh >= character.model.meshCount) continue;

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

        Matrix transform = ComposeTransform(worldPosition, worldRotation, worldScale);
        if (StartsWith(node.name, "VISUAL_")) {
            character.visualParts.push_back(CharacterRenderPart{node.name, node.mesh, transform});
        } else if (StartsWith(node.name, "ICON_")) {
            character.iconParts.push_back(CharacterRenderPart{node.name, node.mesh, transform});
        } else if (StartsWith(node.name, "COL_")) {
            MeshBounds bounds;
            bounds.center = Vector3{0.0f, 0.0f, 0.0f};
            bounds.size = Vector3{1.0f, 1.0f, 1.0f};
            if (node.mesh >= 0 && node.mesh < static_cast<int>(meshBounds.size())) bounds = meshBounds[node.mesh];
            Vector3 scaledLocalCenter = Vector3{
                bounds.center.x * worldScale.x,
                bounds.center.y * worldScale.y,
                bounds.center.z * worldScale.z};
            Vector3 center = Vector3Add(worldPosition, Vector3RotateByQuaternion(scaledLocalCenter, worldRotation));
            Vector3 size = Vector3{
                fabsf(bounds.size.x * worldScale.x),
                fabsf(bounds.size.y * worldScale.y),
                fabsf(bounds.size.z * worldScale.z)};
            float radius = std::max(size.x, size.z) * 0.5f;
            float bodyHeight = std::max(0.0f, size.y - radius * 2.0f);
            Vector3 up = Vector3RotateByQuaternion(Vector3{0.0f, 1.0f, 0.0f}, worldRotation);
            character.colliders.push_back(CharacterCapsule{
                node.name,
                Vector3Subtract(center, Vector3Scale(up, bodyHeight * 0.5f)),
                Vector3Add(center, Vector3Scale(up, bodyHeight * 0.5f)),
                radius});
        }
    }
}

}  // namespace

// ─── Public API ──────────────────────────────────────────────────────────────

bool LoadInteractableCharacter(const std::string& configPath, InteractableCharacter& character) {
    char* raw = LoadFileText(Utils::ResolveProjectPath(configPath).c_str());
    if (raw == nullptr) {
        TraceLog(LOG_WARNING, "Character config: failed to load %s", configPath.c_str());
        return false;
    }

    std::string json = raw;
    UnloadFileText(raw);

    JsonParser parser(json);
    JsonValue root = parser.Parse();
    if (parser.HadError()) {
        TraceLog(LOG_WARNING, "Character config: JSON parse warning in %s: %s", configPath.c_str(), parser.Error().c_str());
    }
    if (root.type != JsonValue::Object) return false;

    const JsonValue* characterConfig = GetMember(root, "character");
    if (!characterConfig || characterConfig->type != JsonValue::Object) return false;

    character = {};
    character.id = StringMember(*characterConfig, "id", "character");
    character.displayName = StringMember(*characterConfig, "display_name", "Personagem");
    character.dialogueText = StringMember(*characterConfig, "dialogue_text", "Texto placeholder.");
    character.modelPath = Utils::ResolveProjectPath(StringMember(*characterConfig, "model_path", ""));
    character.choices = ExtractChoices(*characterConfig);
    character.hasModel = false;
    if (!character.modelPath.empty() && FileExists(character.modelPath.c_str())) {
        character.model = LoadModel(character.modelPath.c_str());
        character.hasModel = character.model.meshCount > 0;
    }

    if (!character.hasModel) return false;

    character.rootPosition = Vector3Zero();
    character.rootRotation = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
    ParseCharacterModelMetadata(character);
    character.originalVisualParts = character.visualParts;
    character.originalIconParts = character.iconParts;
    character.originalColliders = character.colliders;
    if (!character.visualParts.empty() && !character.colliders.empty()) return true;

    TraceLog(LOG_WARNING,
        "Character '%s' skipped: GLB must contain at least one VISUAL_* mesh and one COL_* capsule mesh: %s",
        character.id.c_str(),
        character.modelPath.c_str());
    UnloadModel(character.model);
    character = {};
    return false;
}

void ApplyCharacterRootTransform(InteractableCharacter& character, Vector3 position, Quaternion rotation) {
    character.rootPosition = position;
    character.rootRotation = rotation;
    Matrix rootTransform = MatrixMultiply(
        QuaternionToMatrix(rotation),
        MatrixTranslate(position.x, position.y, position.z));

    character.visualParts = character.originalVisualParts;
    for (std::size_t i = 0; i < character.visualParts.size(); ++i) {
        character.visualParts[i].transform = MatrixMultiply(character.originalVisualParts[i].transform, rootTransform);
    }

    character.iconParts = character.originalIconParts;
    for (std::size_t i = 0; i < character.iconParts.size(); ++i) {
        character.iconParts[i].transform = MatrixMultiply(character.originalIconParts[i].transform, rootTransform);
    }

    character.colliders = character.originalColliders;
    for (std::size_t i = 0; i < character.colliders.size(); ++i) {
        character.colliders[i].bottom = Vector3Add(position, Vector3RotateByQuaternion(character.originalColliders[i].bottom, rotation));
        character.colliders[i].top = Vector3Add(position, Vector3RotateByQuaternion(character.originalColliders[i].top, rotation));
        character.colliders[i].radius = character.originalColliders[i].radius;
    }
}

void DrawCharacterDebugSelection(const InteractableCharacter& character) {
    Vector3 p = character.rootPosition;
    if (!character.colliders.empty()) {
        p = Vector3Scale(Vector3Add(character.colliders[0].bottom, character.colliders[0].top), 0.5f);
    }
    DrawLine3D(p, Vector3Add(p, Vector3{1.0f, 0.0f, 0.0f}), RED);
    DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 1.0f, 0.0f}), GREEN);
    DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 0.0f, 1.0f}), BLUE);
    for (std::size_t i = 0; i < character.colliders.size(); ++i) {
        DrawCapsuleWires(
            character.colliders[i].bottom, character.colliders[i].top,
            character.colliders[i].radius, 12, 6, ORANGE);
    }
}
