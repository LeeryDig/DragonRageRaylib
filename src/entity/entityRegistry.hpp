#ifndef ENTITY_ENTITY_REGISTRY_HPP
#define ENTITY_ENTITY_REGISTRY_HPP

#include <vector>

#include "raylib.h"

#include "entity/entityId.hpp"
#include "input/inputMap.hpp"
#include "interactionSystem.hpp"

struct EntityRegistry {
    EntityId nextId;
    std::vector<EntityId> characterIds;
    std::vector<InteractableCharacter> characters;
    EntityId focusedId;
    EntityId activeDialogueId;
    int selectedChoiceIndex;
    bool dialogueOpen;
};

EntityRegistry CreateEntityRegistry();
void DestroyEntityRegistry(EntityRegistry& registry);
EntityRegistry LoadEntityRegistry(const std::vector<CharacterSpawnConfig>& configs);

EntityId AddCharacter(EntityRegistry& registry, InteractableCharacter character);
void RemoveCharacter(EntityRegistry& registry, EntityId id);
InteractableCharacter* FindCharacter(EntityRegistry& registry, EntityId id);
const InteractableCharacter* FindCharacter(const EntityRegistry& registry, EntityId id);
int CharacterCount(const EntityRegistry& registry);
InteractableCharacter& CharacterAt(EntityRegistry& registry, int index);
const InteractableCharacter& CharacterAt(const EntityRegistry& registry, int index);
EntityId CharacterIdAt(const EntityRegistry& registry, int index);
int IndexOfCharacter(const EntityRegistry& registry, EntityId id);

void UpdateEntityFocus(EntityRegistry& registry, const Camera& camera, float rayLength);
void BeginFocusedDialogue(EntityRegistry& registry);
void UpdateDialogueInput(EntityRegistry& registry, const InputMap& inputMap);
void ResolveCharacterCollisions(EntityRegistry& registry, Vector3& playerPosition, float playerRadius);
void DrawEntityCharacters(const EntityRegistry& registry);
void DrawInteractionUi(const EntityRegistry& registry);

#endif
