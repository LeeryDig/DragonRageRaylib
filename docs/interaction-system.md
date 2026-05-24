# Sistema de Interação e Diálogo

Módulo: `src/interactionSystem.hpp/cpp`

## Estruturas

```cpp
struct InteractableObject {
    std::string id;
    std::string displayName;
    std::string dialogueText;
    Vector3 position;
    Vector3 size;
    Vector3 iconOffset;
    bool focused;
};

struct InteractionSystem {
    std::vector<InteractableObject> objects;
    int focusedIndex;
    int activeDialogueIndex;
    bool dialogueOpen;
};
```

## Dados: `resources/config/interactables.json`

```json
{
  "interactables": [
    {
      "id": "test_box",
      "type": "dialogue",
      "display_name": "Nome",
      "dialogue_text": "Texto do diálogo.",
      "position": [0.0, 0.5, -4.0],
      "size": [1.0, 1.0, 1.0],
      "icon_offset": [0.0, 0.8, 0.0]
    }
  ]
}
```

Tipos suportados: `dialogue`, `inspect_car`, `minigame`, `smoke_spot`, `race_challenge`, `generic`.

## Raycast

```cpp
Ray ray;
ray.position = camera.position;
ray.direction = Normalize(camera.target - camera.position);

BoundingBox box;
box.min = position - size * 0.5f;
box.max = position + size * 0.5f;
RayCollision hit = GetRayCollisionBox(ray, box);
```

Condição de foco:
```
hit.hit == true
hit.distance <= interaction_ray_length     // person.json
distance(person.pos, obj.pos) <= interaction_distance   // person.json
```

`interaction_ray_length` e `interaction_distance` vêm de `person.json` via `PersonConfig`.

## Indicador visual

Esfera amarela acima do objeto quando focado:
```cpp
DrawSphere(object.position + object.iconOffset, 0.12f, YELLOW);
```

## Caixa de diálogo

Layout no bottom da tela:
```
+--------------------------------------------------------------+
| [ portrait ]  Nome do personagem/objeto                      |
|                                                              |
|              Texto do diálogo aqui...                        |
|                                                              |
+--------------------------------------------------------------+
```

Desenhado após `EndMode3D()`. Fundo escuro semitransparente, portrait placeholder, nome em destaque, texto com word wrap.

Input: `E` abre diálogo (se objeto focado) ou fecha (se diálogo aberto).
