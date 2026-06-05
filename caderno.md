# Coisa para fazer

## animações
    Ainda precisa pensar em tudo que vai precisar fazer para animações decidir como vai implementar isso:
    Engines usam 3 camadas:

 1. Asset
     - Arquivo/modelo tem animações: idle, walk, run, attack, jump
     - Pode vir tudo num .glb, ou cada animação em arquivo separado
     - Importador cria clips internos
 2. Animator / Animation Player
     - Guarda lista de clips carregados
     - Toca 1 clip ou mistura vários
     - Ex: play("walk"), crossfade("run", 0.2s)
 3. State Machine / Animation Tree
     - Decide qual clip tocar conforme estado do jogo
     - Ex:
         - velocidade 0 → idle
         - velocidade > 0 → walk/run
         - apertou ataque → attack
         - no ar → jump/fall

 Fluxo comum:

 ```txt
   Carrega personagem
     → carrega skeleton/model
     → carrega clips essenciais
     → começa idle

   Loop jogo
     → lê estado gameplay
     → escolhe animação desejada
     → se mudou, faz blend/crossfade
     → avança frame
     → aplica pose no skeleton
     → renderiza mesh deformada
 ```

 Unity/Godot normalmente fazem:
 - modelo/skeleton carregado como recurso principal
 - animações viram AnimationClip
 - controller/tree referencia clips
 - engine carrega dependências quando cena/prefab/node entra
 - pode lazy-load/addressables/resource packs em jogo maior

 Primeiro MVP:
 - 1 animação por vez
 - sem blend
 - estados: idle/walk/talk
 - carregar todas animações do GLB junto

 Depois:
 - crossfade
 - blend walk/run
 - layer superior
 - lazy loading se precisar.

## feature de conversar com os NPCs
    - Precisa ter um sistema de arvore de dialogos

## feature de apostar nas corridas que vai ter ao redor
    - Se o player quiser ele vai poder apostar as corridas que vai ta rolando ao redor


## Ideias
    - No começo da noite ainda não vai rolar nada pois o pessoal ainda vai estar chegando, dai você vai conseguir ver os carros conversar com as pessoas e entender melhor o movimento, para que quando vc quiser apostar você tenha uma noção de o quem é melhor ali
    - Ainda precisa de uma ideia mais concreta de game loop, eu tenho as conversas com o pessoal, e tenho as apostas mas você vai poder fazer mais o que?

Eixos de design pra você pensar sozinho depois
Antes das ideias, te entrego o "molde" — três perguntas que transformam qualquer elemento de cenário em mecânica:

Essa coisa pode virar uma transação? (eu dou X, recebo Y)
Essa coisa pode virar uma fofoca/informação? (saber algo que outro não sabe)
Essa coisa pode virar um relacionamento que evolui? (esse NPC me trata diferente depois)

Quase tudo num meet noturno passa por um desses três filtros. Vou usar eles agora.

## Sistema de shader de blur:
###  eu quero fazer um sistema para aplicar blur nas placas dos carros
    - Tem que ser um sistema que quando a camera pegar um ponto que eu irei marcar vai aplicar um blur quadriculado
    - Sera um quad que eu vou poder definir o tamanho e ele vai dar o efeito de blur só no tamanho dele
    - tem que ser algo do tamanho que eu determinar no blender
    - Eu vou poder escolher ele dentro da lista de props similar ao que temos para selecionar luzes


