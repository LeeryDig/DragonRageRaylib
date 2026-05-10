# Plano de Implementação — Interação por Ray e Caixa de Diálogo

## Contexto

No modo first person dos Night Meets, o jogador precisa olhar para objetos/NPCs e interagir com eles. A interação não deve acontecer só por proximidade: o jogador precisa mirar/olhar para o alvo com o ray da câmera.

Antes de implementar, este documento define como o sistema deve funcionar.

## Objetivo

Criar um sistema inicial onde:

- Um ray sai da câmera em first person.
- Se o ray acertar um objeto interativo dentro do range, aparece um indicador visual.
- O indicador visual inicial pode ser uma bolinha/ícone acima do objeto.
- Ao apertar o botão de interação, aparece uma caixa de diálogo no bottom da tela.
- O diálogo mostra:
  - imagem/portrait placeholder;
  - nome do personagem/objeto;
  - texto configurável.

## Fluxo esperado

1. Jogador anda pelo level em first person.
2. Ray sai do centro da câmera/tela.
3. Ray passa/acerta uma entidade interativa.
4. Sistema checa se o alvo está dentro do range.
5. Se estiver, aparece uma bolinha/ícone acima do objeto.
6. Jogador aperta `E`.
7. Abre caixa de diálogo na parte inferior da tela.
8. Caixa mostra nome e texto daquele objeto.
9. Jogador aperta `E`, `Space` ou outro botão para fechar/avançar.

## Interação por ray

A interação será baseada em raycast da câmera.

Condição para um objeto ser considerado focado:

```txt
ray da câmera acerta bounds do objeto
objeto tem flag/type de interativo
personagem está dentro de interaction_distance
```

O ray pode ser criado a partir da câmera atual:

```cpp
Ray ray;
ray.position = camera.position;
ray.direction = Normalize(camera.target - camera.position);
```

Depois testamos contra uma bounding box ou esfera de interação.

## Objeto interativo inicial

Para o primeiro protótipo, podemos começar com uma caixa qualquer no cenário.

Ela teria:

- posição;
- tamanho/bounds;
- nome;
- texto de diálogo;
- posição do ícone/bolinha;
- estado de foco.

Conceito:

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
```

## Indicador visual de interação

A ideia inicial é usar uma bolinha/ícone acima do objeto.

Comportamento:

- Normalmente invisível.
- Quando o ray estiver mirando o objeto e ele estiver em range, aparece.
- Pode ficar acima da caixa/NPC.
- Pode ser uma esfera simples no mundo 3D no primeiro protótipo.

Exemplo visual inicial:

```cpp
DrawSphere(object.position + object.iconOffset, 0.12f, YELLOW);
```

Mais tarde isso pode virar:

- sprite 2D billboard;
- ícone com tecla `E`;
- animação de pulso;
- highlight no objeto;
- contorno/outline.

## Caixa de diálogo

Ao interagir, deve aparecer uma caixa grande na parte inferior da tela.

Layout desejado:

```txt
+--------------------------------------------------------------+
| [ portrait ]  Nome do personagem/objeto                      |
|                                                              |
|              Texto do diálogo aqui...                        |
|                                                              |
+--------------------------------------------------------------+
```

Características:

- Fica no bottom da tela.
- Ocupa boa parte da largura.
- Tem área para portrait/imagem placeholder.
- Mostra nome em destaque.
- Mostra texto abaixo.
- Pode ter fundo escuro/semitransparente.
- Pode ter borda simples.

No primeiro protótipo, a portrait pode ser:

- retângulo placeholder;
- textura padrão;
- ícone genérico;
- cor sólida com texto `IMG`.

## Dados configuráveis

Sim, o nome e texto devem ser configuráveis. Isso evita hardcode e prepara o sistema para NPCs, carros e objetos diferentes.

Arquivo sugerido inicial:

```txt
resources/config/interactables.json
```

Ou, futuramente, por level:

```txt
resources/levels/night_meet_01_interactables.json
```

Para começar, `resources/config/interactables.json` é suficiente.

## Exemplo de JSON

```json
{
  "interactables": [
    {
      "id": "test_box",
      "type": "dialogue",
      "display_name": "Caixa Misteriosa",
      "dialogue_text": "Essa caixa parece estar guardando alguma coisa importante.",
      "position": [0.0, 0.5, -4.0],
      "size": [1.0, 1.0, 1.0],
      "icon_offset": [0.0, 0.8, 0.0]
    }
  ]
}
```

Campos:

- `id`: identificador interno.
- `type`: tipo da interação.
- `display_name`: nome mostrado na caixa de diálogo.
- `dialogue_text`: texto mostrado ao interagir.
- `position`: posição do objeto.
- `size`: tamanho usado para desenhar/testar bounds.
- `icon_offset`: offset para posicionar a bolinha/ícone acima do objeto.

## Tipos de interação futuros

Mesmo que agora só exista diálogo, o sistema deve nascer preparado para outros tipos.

Tipos possíveis:

```txt
dialogue
inspect_car
minigame
smoke_spot
race_challenge
generic
```

Inicialmente, só implementamos:

```txt
dialogue
```

## Estrutura técnica sugerida

Arquivos possíveis:

```txt
src/interactionSystem.hpp
src/interactionSystem.cpp
src/dialogueUi.hpp
src/dialogueUi.cpp
```

Para o primeiro protótipo, podemos fazer simples:

```txt
src/interactionSystem.hpp
src/interactionSystem.cpp
```

Esse sistema cuidaria de:

- carregar `interactables.json`;
- guardar lista de objetos interativos;
- fazer raycast contra bounds;
- determinar objeto focado;
- abrir/fechar diálogo;
- desenhar indicadores 3D;
- desenhar caixa de diálogo 2D.

## Estado do sistema

Conceito:

```cpp
struct InteractionSystem {
    std::vector<InteractableObject> objects;
    int focusedIndex;
    int activeDialogueIndex;
    bool dialogueOpen;
};
```

Regras:

- `focusedIndex`: objeto atualmente mirado pelo ray.
- `activeDialogueIndex`: objeto cujo diálogo está aberto.
- `dialogueOpen`: se a caixa de diálogo está visível.

## Input

Tecla sugerida:

```txt
E: interagir / avançar / fechar diálogo
```

Comportamento inicial:

- Se diálogo está fechado e existe objeto focado: `E` abre diálogo.
- Se diálogo está aberto: `E` fecha diálogo.

Mais tarde podemos separar:

- `E`: interagir;
- `Space`: avançar texto;
- `Esc`: fechar.

## Raycast contra objeto

Para uma caixa simples, podemos usar `GetRayCollisionBox` da raylib.

Exemplo conceitual:

```cpp
BoundingBox box;
box.min = position - size * 0.5f;
box.max = position + size * 0.5f;
RayCollision hit = GetRayCollisionBox(ray, box);
```

Depois validar:

```txt
hit.hit == true
hit.distance <= interaction_ray_length
distance(person.position, object.position) <= interaction_distance
```

`interaction_ray_length` e `interaction_distance` podem vir do `person.json`.

## Integração com person.json

Precisamos adicionar/usar parâmetros no `person.json`:

```json
{
  "interaction_distance": 2.0,
  "interaction_ray_length": 4.0
}
```

`interaction_ray_length` limita até onde o ray pode detectar.

`interaction_distance` garante que o jogador não interaja de muito longe, mesmo mirando no objeto.

## Renderização 3D

Enquanto o diálogo estiver fechado:

- Desenhar o level.
- Desenhar objetos interativos de teste.
- Se algum estiver focado, desenhar a bolinha/ícone acima dele.

A bolinha deve aparecer apenas no foco atual.

## Renderização 2D

Depois do `EndMode3D()`, desenhar UI:

- Prompt simples opcional: `E Interagir`.
- Caixa de diálogo se aberta.

Exemplo:

```cpp
DrawRectangle(... fundo ...);
DrawRectangle(... portrait ...);
DrawText(nome, ...);
DrawText(texto, ...);
```

Para quebra de linha do texto, pode começar simples e depois evoluir para word wrap.

## Plano de implementação sugerido

### Etapa 1 — Config

- Adicionar `interaction_distance` e `interaction_ray_length` ao `person.json`/`PersonConfig`.
- Criar `resources/config/interactables.json` com uma caixa teste.

### Etapa 2 — Sistema de interação

- Criar `interactionSystem.hpp/cpp`.
- Carregar lista de interativos do JSON.
- Implementar raycast da câmera contra bounding boxes.
- Guardar `focusedIndex`.

### Etapa 3 — Indicador visual

- Desenhar a caixa teste no mundo.
- Desenhar bolinha amarela acima dela quando focada.

### Etapa 4 — Abertura de diálogo

- Ao apertar `E`, abrir diálogo do objeto focado.
- Enquanto diálogo estiver aberto, opcionalmente bloquear movimento/câmera.

### Etapa 5 — UI de diálogo

- Desenhar caixa no bottom da tela.
- Desenhar portrait placeholder.
- Desenhar nome.
- Desenhar texto.

### Etapa 6 — Refinamento

- Melhorar visual do indicador.
- Adicionar quebra de linha no texto.
- Adicionar múltiplas páginas de diálogo.
- Adicionar portrait real por config.
- Integrar com NPCs e carros.

## Perguntas em aberto

- Enquanto diálogo está aberto, o jogador pode mover a câmera ou fica travado?
- A bolinha deve ficar em 3D no mundo ou ser um ícone 2D projetado na tela?
- A caixa de diálogo fecha com `E`, `Space` ou `Esc`?
- O texto será uma única fala ou uma sequência de falas?
- O portrait será por personagem/NPC ou por tipo de objeto?

## Decisão atual sugerida

Para o primeiro protótipo:

- Usar `resources/config/interactables.json`.
- Criar uma caixa teste interativa.
- Usar ray da câmera.
- Usar `GetRayCollisionBox`.
- Mostrar uma esfera amarela acima do objeto quando focado.
- `E` abre/fecha diálogo.
- Caixa de diálogo no bottom com portrait placeholder, nome e texto.
