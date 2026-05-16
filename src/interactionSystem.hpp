#ifndef INTERACTION_SYSTEM_HPP
#define INTERACTION_SYSTEM_HPP

#include <string>
#include <vector>

#include "raylib.h"

struct InteractionChoice {
    std::string text;
};

struct CharacterRenderPart {
    std::string name;
    int meshIndex;
    Matrix transform;
};

struct CharacterCapsule {
    std::string name;
    Vector3 bottom;
    Vector3 top;
    float radius;
};

struct InteractableCharacter {
    std::string id;
    std::string displayName;
    std::string dialogueText;
    std::vector<InteractionChoice> choices;
    std::string modelPath;
    Model model;
    bool hasModel;
    Vector3 rootPosition;
    Quaternion rootRotation;
    std::vector<CharacterRenderPart> visualParts;
    std::vector<CharacterRenderPart> iconParts;
    std::vector<CharacterCapsule> colliders;
    std::vector<CharacterRenderPart> originalVisualParts;
    std::vector<CharacterRenderPart> originalIconParts;
    std::vector<CharacterCapsule> originalColliders;
};

struct InteractionSystem {
    std::vector<InteractableCharacter> characters;
    int focusedIndex;
    int activeDialogueIndex;
    int selectedChoiceIndex;
    bool dialogueOpen;
};

InteractionSystem LoadInteractionSystem(const std::string& configPath);
void UnloadInteractionSystem(InteractionSystem& system);

void UpdateInteractionFocus(
    InteractionSystem& system,
    const Camera& camera,
    Vector3 playerPosition,
    float interactionDistance,
    float rayLength);

void BeginFocusedDialogue(InteractionSystem& system);
void UpdateDialogueInput(InteractionSystem& system);
void ResolveCharacterCollisions(InteractionSystem& system, Vector3& playerPosition, float playerRadius);

void DrawInteractableCharacters(const InteractionSystem& system);
void DrawInteractionUi(const InteractionSystem& system);
void ApplyCharacterRootTransform(InteractableCharacter& character, Vector3 position, Quaternion rotation);
void DrawCharacterDebugSelection(const InteractableCharacter& character);

#endif
