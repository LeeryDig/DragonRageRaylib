# Plano de Implementação — Controle de Personagem a Pé

## Contexto

Para os **Night Meets**, o jogador precisa sair do modo de carro e controlar um personagem a pé dentro do encontro. Esse personagem será usado para explorar o local, conversar com pessoas, interagir com objetos e executar ações simples como fumar.

Este documento descreve como podemos implementar esse controle usando o que o projeto já tem hoje: **C++**, **raylib/raymath**, sistema de input atual, câmera, física/colisão própria e arquivos de configuração em JSON.

Por enquanto, este arquivo é apenas planejamento. A implementação fica para uma próxima etapa.

## Objetivo

Criar um modo de controle de personagem com suporte a:

- Andar pelo cenário.
- Virar/mover o personagem conforme input.
- Interagir com NPCs, carros e objetos.
- Executar ação de fumar cigarro.
- Ler parâmetros do personagem a partir de um arquivo JSON.
- Servir como base para diálogos, minigames, inspeção de carros e outras interações futuras.

## Escopo inicial

A primeira versão deve ser simples e funcional.

Inclui:

- Movimento básico no chão.
- Colisão simples com o ambiente.
- Detecção de interação por **raycast saindo da câmera**.
- Checagem de range antes de permitir a interação.
- Botão de interação.
- Botão/ação para fumar.
- Sistema de cigarro aceso na mão, com baforadas e desgaste.
- Contador de cigarros.
- Configuração via JSON.

Não inclui ainda:

- Animação complexa do personagem.
- Inventário completo.
- Sistema final de diálogo.
- Minigames reais.
- IA de NPCs.
- Transição final entre meet e corrida.
- Customização visual do personagem.

## Nome do arquivo de configuração

Assim como o projeto já possui:

```txt
resources/config/vehicle.json
```

Vamos criar um arquivo equivalente para o personagem.

Nome sugerido:

```txt
resources/config/person.json
```

Motivo para usar `person.json`:

- Curto e direto.
- Combina com a ideia de personagem controlável a pé.
- Evita confundir com `player`, que futuramente pode representar tanto carro quanto personagem.
- Deixa claro que são parâmetros do corpo/personagem humano.

Alternativas possíveis:

- `character.json`
- `player_character.json`
- `on_foot.json`

Decisão sugerida por enquanto: **`person.json`**.

## Parâmetros iniciais do `person.json`

Exemplo conceitual:

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

### Campos

- `walk_speed`: velocidade normal andando.
- `acceleration`: rapidez para atingir a velocidade desejada.
- `deceleration`: rapidez para parar quando não há input.
- `turn_speed`: velocidade com que o personagem vira para a direção do movimento.
- `interaction_distance`: distância máxima real entre personagem e alvo para permitir interação.
- `interaction_ray_length`: comprimento máximo do ray que sai da câmera para encontrar o alvo olhado.
- `cigarette_burn_duration`: tempo que um cigarro dura aceso se o jogador não baforar.
- `cigarette_puff_wear`: quanto tempo de vida do cigarro é consumido a cada baforada.
- `puff_cooldown`: intervalo mínimo entre baforadas.
- `max_cigarettes`: máximo de cigarros carregados.
- `starting_cigarettes`: cigarros iniciais ao entrar no meet.
- `collider_size`: tamanho aproximado do colisor do personagem.
- `collider_offset`: offset do colisor em relação à posição base.
- `camera_distance`: distância da câmera em relação ao personagem.
- `camera_height`: altura da câmera.
- `camera_smooth`: suavização da câmera.

## Estrutura técnica sugerida

Podemos criar um sistema separado para o personagem, seguindo o padrão do projeto.

Arquivos possíveis:

```txt
src/personController.hpp
src/personController.cpp
```

Ou, se quisermos separar melhor no futuro:

```txt
src/player/personController.hpp
src/player/personController.cpp
src/player/personConfig.hpp
src/player/personConfig.cpp
```

Para uma primeira implementação, a opção simples provavelmente é suficiente:

```txt
src/personController.hpp
src/personController.cpp
```

## Dados principais do personagem

O controlador do personagem pode guardar:

```txt
position
velocity
forward_direction
is_interacting
has_lit_cigarette
cigarette_life_remaining
puff_cooldown_timer
smoke_puff_count
cigarettes_count
current_interactable
```

Conceitualmente:

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

O movimento pode ser feito usando `raylib`/`raymath`:

- `IsKeyDown()` para input.
- `Vector3` para posição e direção.
- Funções de `raymath` para normalizar, somar e interpolar vetores.
- `GetFrameTime()` ou timestep fixo dependendo de como o loop principal estiver organizado.

Input inicial sugerido:

```txt
W / S: mover para frente/trás
A / D: mover para esquerda/direita
E: interagir
F: acender cigarro / baforar se já estiver aceso
```

O movimento pode ser relativo à câmera ou ao mundo.

### Opção 1 — Movimento relativo ao mundo

Mais simples para protótipo.

- W move no eixo Z.
- S move no eixo -Z.
- A/D movem no eixo X.

Vantagem:

- Fácil de implementar.

Desvantagem:

- Pode parecer menos natural dependendo da câmera.

### Opção 2 — Movimento relativo à câmera

Mais adequado para exploração em terceira pessoa.

- W anda para onde a câmera aponta.
- A/D fazem strafe relativo à câmera.
- O personagem vira para a direção do movimento.

Vantagem:

- Melhor sensação de controle.

Desvantagem:

- Exige cálculo de forward/right da câmera no plano horizontal.

Decisão sugerida: começar com movimento relativo à câmera, pois o Night Meet será exploração em terceira pessoa.

## Colisão

O projeto já tem um sistema próprio de física/colisão em `src/physics`.

Para o personagem, podemos começar de forma simples:

- Representar o personagem com um colisor tipo caixa/cápsula aproximada.
- Usar uma `PhysicsBody` ou uma estrutura simples com checagem contra o mundo estático.
- Impedir o personagem de atravessar paredes, carros e objetos grandes.

Como primeira versão, não precisa ter física realista. O personagem só precisa:

- Ficar no chão.
- Não atravessar obstáculos.
- Deslizar ou parar ao bater em colisores.

Se o sistema atual de física ainda não estiver ideal para personagem, podemos implementar inicialmente uma resolução simples:

1. Calcular posição desejada.
2. Testar colisão com obstáculos.
3. Se colidiu, cancelar ou ajustar o movimento.
4. Atualizar posição final.

## Câmera

O personagem precisa de uma câmera diferente da câmera do carro.

Opção inicial:

- Câmera em terceira pessoa atrás do personagem.
- Altura e distância configuradas em `person.json`.
- Suavização simples com interpolação.

Parâmetros vindos do JSON:

```txt
camera_distance
camera_height
camera_smooth
```

A câmera pode olhar para:

```txt
person.position + Vector3{0, camera_height, 0}
```

E ficar atrás da direção do personagem ou em uma direção controlada pelo mouse no futuro.

Para o primeiro protótipo, podemos manter simples:

- Câmera segue o personagem.
- Câmera aponta para ele.
- Sem rotação livre com mouse inicialmente, se isso acelerar a implementação.

## Sistema de interação

A interação será uma ação genérica. Apertar o botão de interação pode causar várias coisas dependendo do alvo:

- Abrir diálogo com NPC.
- Inspecionar carro.
- Iniciar minigame.
- Ativar objeto do cenário.
- Mostrar texto descritivo.
- Futuramente escolher aposta ou desafio.

### Regra principal de detecção

A detecção de interação será feita com um **ray saindo da câmera**.

Funcionamento esperado:

1. A câmera gera um ray apontando para frente, na direção do centro da tela/mira.
2. O sistema testa esse ray contra objetos interativos.
3. Se o ray acertar um objeto interativo, o sistema checa a distância entre o personagem e o alvo.
4. Se o alvo estiver dentro de `interaction_distance`, mostra o prompt.
5. Ao apertar `E`, dispara a ação daquele objeto.

Isso evita interação automática apenas por proximidade. O jogador precisa olhar/apontar para o item ou NPC que quer usar.

### Interface conceitual

Podemos pensar em um tipo comum para objetos interativos:

```cpp
struct Interactable {
    Vector3 position;
    Vector3 boundsSize;
    const char* prompt;
    InteractionType type;
};
```

Tipos possíveis:

```txt
Dialogue
InspectCar
SmokeSpot
Minigame
Generic
```

No protótipo inicial, não precisa existir uma arquitetura complexa. Podemos começar com uma lista simples de objetos interativos no cenário e testar o ray contra bounding boxes/esferas simples.

### Detecção de interação por ray

Dados usados:

- Ray da câmera.
- `interaction_ray_length` para limitar o alcance do ray.
- `interaction_distance` para garantir que o personagem está perto o suficiente do alvo.
- Flag/tipo indicando se o objeto é interativo.

Condição para interagir:

```txt
ray acertou objeto
objeto é interativo
distância personagem -> objeto <= interaction_distance
```

Decisão: usar ray da câmera + checagem de range.

## Sistema de fumar

O personagem poderá fumar durante o encontro, mas não será uma ação única de alguns segundos. A ideia é que o cigarro fique **aceso na mão** por um tempo.

Regras iniciais:

- Botão sugerido para acender/começar: `F`.
- Só pode acender se tiver cigarros disponíveis.
- Ao acender, reduz `cigarettes_count` em 1.
- O cigarro passa a existir como estado ativo na mão do personagem.
- O cigarro queima sozinho ao longo do tempo usando `cigarette_burn_duration`.
- O jogador pode baforar enquanto o cigarro está aceso.
- Cada baforada desgasta o cigarro usando `cigarette_puff_wear`.
- Quanto mais o jogador bafora, mais rápido o cigarro termina.
- `puff_cooldown` evita baforadas infinitas por segundo.
- Quando a vida do cigarro chega a zero, ele apaga/acaba.

Estados possíveis do cigarro:

```txt
sem cigarro ativo
cigarro aceso na mão
baforando
cigarro acabou
```

No primeiro protótipo, o efeito pode ser apenas registrado em estado interno, por exemplo:

```txt
has_smoked_recently = true
smoke_intensity = quantidade_de_baforadas
```

Depois esse estado pode ser usado pelo sistema de corrida.

Possíveis efeitos futuros:

- Reduzir nervosismo/erro em largada.
- Aumentar foco por alguns minutos.
- Melhorar controle em corrida por pouco tempo.
- Dar alguma penalidade se usado demais.

Ainda não vamos fechar o balanceamento.

## Estados do personagem

Podemos organizar o personagem em estados simples:

```txt
Idle
Walking
Interacting
Smoking
```

Regras:

- `Idle`: sem input de movimento.
- `Walking`: input de movimento normal.
- `Interacting`: durante uma interação que trava movimento.
- `Smoking`: durante a ação de fumar.

Isso facilita controlar quando o jogador pode ou não se mover.

## Integração com Night Meets

O controle de personagem será usado principalmente dentro do modo Night Meet.

Fluxo esperado:

1. Jogador entra no meet.
2. Sistema carrega `person.json`.
3. Personagem é spawnado em um ponto do cenário.
4. Câmera muda para modo personagem.
5. Jogador anda pelo local.
6. Sistema usa um ray da câmera para detectar o interativo que o jogador está olhando.
7. Jogador aperta interação.
8. Dependendo do objeto, abre diálogo, inspeção ou outra ação.
9. Jogador pode fumar durante o encontro.
10. Ao fim do meet, o controle pode voltar para outro modo de jogo.

## Como fazer usando o que já existe

Com as dependências atuais, já temos praticamente o necessário para o primeiro protótipo:

### raylib

Usar para:

- Input de teclado.
- Desenho/debug visual.
- Câmera 3D.
- Geração de ray da câmera para interação.
- Testes simples de colisão do ray com bounds de objetos interativos.
- Carregamento/renderização de modelos, se necessário.

### raymath

Usar para:

- Vetores `Vector3`.
- Normalização.
- Distâncias.
- Interpolação.
- Cálculo de direção.

### Sistema de física atual

Usar para:

- Colisão com o chão.
- Colisão com obstáculos.
- Possível corpo/colisor do personagem.

### JSON

Seguir o mesmo caminho do `vehicle.json`:

- Criar `resources/config/person.json`.
- Carregar parâmetros no início do modo/personagem.
- Usar defaults caso algum campo não exista.

## Ordem de implementação sugerida

### Etapa 1 — Configuração

- Criar `resources/config/person.json`.
- Criar estrutura `PersonConfig`.
- Implementar carregamento dos parâmetros.

### Etapa 2 — Movimento básico

- Criar `PersonController`.
- Ler input WASD.
- Atualizar posição e velocidade.
- Fazer personagem virar para direção do movimento.

### Etapa 3 — Câmera

- Criar câmera simples seguindo o personagem.
- Usar parâmetros do `person.json`.

### Etapa 4 — Interação genérica

- Criar lista simples de interativos.
- Gerar ray a partir da câmera.
- Testar ray contra objetos interativos.
- Checar se o objeto acertado está dentro de `interaction_distance`.
- Mostrar prompt na tela.
- Executar callback/ação placeholder ao apertar `E`.

### Etapa 5 — Fumar

- Adicionar contador de cigarros.
- Implementar ação de acender cigarro com tecla `F`.
- Manter cigarro aceso na mão com tempo de vida.
- Implementar baforada com cooldown.
- Fazer cada baforada gastar parte da vida do cigarro.
- Registrar estado/efeito para uso futuro.

### Etapa 6 — Colisão

- Integrar com o sistema de física/colisão do projeto.
- Impedir atravessar paredes, carros e objetos.

### Etapa 7 — Integração com Night Meets

- Usar personagem como controle principal dentro do meet.
- Adicionar NPCs/carros como interativos.
- Preparar ligação com diálogos e eventos.

## Perguntas em aberto

- A câmera será fixa/semi-fixa ou livre com mouse?
- O ray de interação deve sair exatamente do centro da tela ou de uma mira deslocada?
- Fumar pode ser feito em qualquer lugar ou só em pontos específicos?
- O personagem poderá fumar andando ou deve parar para baforar?
- O cigarro terá limite por meet ou por campanha?
- O personagem terá modelo 3D agora ou começa como placeholder/debug shape?
- O controle a pé será usado apenas nos Night Meets ou também em outros modos futuramente?

## Decisão atual

Vamos documentar o controle do personagem como um sistema separado, configurável por JSON, começando por um protótipo simples.

Decisões sugeridas para a primeira implementação:

- Criar `resources/config/person.json`.
- Criar `PersonController`.
- Movimento relativo à câmera.
- Interação por ray da câmera + checagem de range.
- Tecla `E` para interagir.
- Tecla `F` para fumar.
- Cigarros controlados por contador + cigarro ativo com tempo de queima e baforadas.
- Câmera terceira pessoa simples.
