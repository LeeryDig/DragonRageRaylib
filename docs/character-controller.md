# Controle de Personagem a Pé

Sistema usado nos Night Meets. Jogador controla personagem humano explorando o encontro.

## Configuração: `resources/config/person.json`

```json
{
  "walk_speed": 3.0,
  "acceleration": 18.0,
  "deceleration": 22.0,
  "turn_speed": 12.0,
  "interaction_distance": 2.0,
  "interaction_ray_length": 4.0,
  "cigarette_burn_duration": 180.0,
  "cigarette_puff_wear": 12.0,
  "puff_cooldown": 1.0,
  "max_cigarettes": 5,
  "starting_cigarettes": 3,
  "collider_size": [0.6, 1.8, 0.6],
  "collider_offset": [0.0, 0.9, 0.0],
  "camera_distance": 4.0,
  "camera_height": 2.0,
  "camera_smooth": 10.0
}
```

## Estrutura

```cpp
struct PersonController {
    Vector3 position;
    Vector3 velocity;
    Vector3 forward;

    int cigarettes;
    bool hasLitCigarette;
    float cigaretteLifeRemaining;
    float puffCooldownTimer;
    int smokePuffCount;

    PersonConfig config;
};
```

## Movimento

Câmera-relativo: WASD move em relação ao forward/right da câmera no plano horizontal. Personagem vira para direção do movimento com `turn_speed`.

## Câmera

Terceira pessoa atrás do personagem. Parâmetros `camera_distance`, `camera_height`, `camera_smooth` do `person.json`. Target: `person.position + {0, camera_height, 0}`.

## Sistema de interação

Ray sai do centro da câmera. Condição para foco:

```
ray acerta BoundingBox do objeto
distância personagem → objeto ≤ interaction_distance
```

`E`: interagir / avançar / fechar diálogo. Ver [interaction-system.md](interaction-system.md).

## Sistema de cigarro

`F`: acende cigarro (consome 1 do contador).

| Estado | Descrição |
|---|---|
| sem cigarro ativo | nenhum cigarro aceso |
| cigarro aceso | queima sozinho por `cigarette_burn_duration` |
| baforando | `F` enquanto aceso; cada baforada gasta `cigarette_puff_wear`; cooldown por `puff_cooldown` |
| cigarro acabou | vida chegou a zero |

Estado registrado internamente (`has_smoked_recently`, `smoke_intensity`) para uso futuro no sistema de corrida.

## Estados do personagem

| Estado | Condição |
|---|---|
| Idle | sem input de movimento |
| Walking | WASD ativo |
| Interacting | interação travando movimento |
| Smoking | ação de fumar ativa |
