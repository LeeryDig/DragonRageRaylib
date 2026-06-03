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
