---
name: player-raycast-interactions
description: Regra obrigatória para qualquer interação do player no DragonRageRaylib. Use sempre que implementar, revisar ou alterar objetos interativos, prompts de interação, NPCs, rádios, portas, itens, triggers clicáveis ou qualquer ação em que o player precise olhar e apertar/clicar para interagir.
---

# Player Raycast Interactions

## Regra principal

Toda interação ativa do player com objetos do mundo deve usar **um raycast único baseado na câmera/visão do player**.

Se o player precisa **olhar + apertar Interact/clicar**, então a seleção/foco deve ser feita por raycast, não por distância pura.

## Helper oficial

Sempre reutilizar o sistema comum:

```cpp
#include "interaction/interactionRay.hpp"
```

Funções atuais:

```cpp
BuildInteractionRay(camera)
RayHitsInteractionBox(...)
RayHitsInteractionCapsule(...)
DrawInteractionRayDebug(...)
```

Não criar rays paralelos/inconsistentes para cada sistema.

## Padrão obrigatório

1. Criar o ray com:

```cpp
Ray ray = BuildInteractionRay(camera);
```

2. Testar o mesmo ray contra o collider do alvo:
   - `RayHitsInteractionCapsule` para characters/NPCs;
   - `RayHitsInteractionBox` para rádios, portas, props e itens;
   - outro helper comum se surgir novo collider.

3. Aplicar limite de alcance máximo:

```cpp
interactionRayLength
```

4. Só mostrar prompt se o ray acertar o objeto dentro do alcance.
5. Só executar interação se o objeto estiver focado pelo ray.

## Interaction tag / tipo interativo

Todo objeto que o player pode olhar e interagir deve ser tratado como um **Interactable**.

Exemplos:

- character com diálogo;
- rádio;
- porta;
- pickup;
- botão;
- prop clicável.

Cada Interactable deve ter pelo menos:

- tipo/tag de interação;
- collider para raycast;
- texto/prompt de interação;
- ação ao apertar `Interact`.

Mesmo que ainda existam sistemas separados, a regra arquitetural é convergir para um foco único de interação.

## Debug obrigatório

Quando o modo debug `showForces` estiver ativo, o ray de interação deve ser desenhado usando:

```cpp
DrawInteractionRayDebug(camera, interactionRayLength);
```

O tamanho visual do ray deve ser exatamente o alcance real usado pela interação.

## Proibido para interação ativa

Não usar apenas:

```cpp
Vector3Distance(playerPosition, objectPosition) < radius
```

Isso só pode ser filtro secundário/otimização, nunca critério principal.

Também evitar desenhar esfera de interação no gameplay normal. Esferas/volumes só em debug/editor quando necessário.

## Permitido

Distância pode ser usada para:

- áudio/volume espacial;
- ativação passiva por proximidade;
- otimização antes do raycast;
- limite secundário depois do hit do ray.

Mas o foco interativo continua sendo por raycast.

## Checklist

Antes de finalizar qualquer interação:

- [ ] Usa `BuildInteractionRay(camera)`?
- [ ] Usa helper comum para box/cápsula/outro collider?
- [ ] Usa `interactionRayLength` como alcance?
- [ ] O prompt só aparece quando o ray acerta?
- [ ] O `Interact` só executa no objeto focado?
- [ ] O objeto tem tag/tipo de Interactable?
- [ ] `showForces` mostra o mesmo ray e mesmo alcance?
- [ ] Distância pura não é o critério principal?
