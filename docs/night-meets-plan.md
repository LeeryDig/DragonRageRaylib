# Plano de Implementação — Night Meets

## Contexto

Os **Night Meets** serão o momento do jogo em que o jogador participa de encontros noturnos de carros antes das corridas. A ideia é criar uma cena social viva, com carros estacionados, pilotos, grupos de pessoas, conversas, pequenas interações e a sensação de festa/evento clandestino.

Por enquanto, o foco deste documento é **planejar e documentar a mudança**, sem implementar ainda.

## Objetivo da feature

Criar um modo de jogo onde o jogador possa:

- Andar a pé pelo local do encontro.
- Observar carros estacionados.
- Conversar com pilotos e pessoas presentes.
- Interagir com elementos do ambiente.
- Sentir que o encontro é um evento vivo, com pessoas em grupos, conversas e rotinas.
- Preparar terreno para, no futuro, escolher com quem apostar corridas.

## Fora do escopo inicial

Nesta primeira etapa dos Night Meets, **não vamos implementar ainda**:

- Sistema completo de apostas.
- Seleção final de corrida.
- Corridas iniciadas diretamente pelo meet.
- Economia/balanceamento de apostas.
- Consequências de vitória/derrota nas apostas.

A parte de apostas e desafios de corrida deve ser pensada como integração futura. O modo Night Meet inicial é apenas o encontro em si.

## Fantasia e ambientação

O cenário provavelmente será uma garagem, estacionamento ou ponto de encontro próximo ao local onde as corridas acontecem.

Elementos desejados:

- Carros estacionados para o jogador observar.
- Pessoas espalhadas pelo local.
- Grupos de pessoas conversando.
- Grupos associados a oficinas, equipes ou estilos de carro.
- Sensação de festa, evento e movimento.
- Ambiente noturno, começando por volta das 23h ou 00h e terminando às 04h.

Exemplo de composição:

- Um grupo perto de carros preparados por uma oficina específica.
- Pessoas fumando ou conversando perto de uma parede/entrada.
- Pilotos perto dos próprios carros.
- Curiosos olhando motores, rodas ou pintura dos carros.
- NPCs mudando de posição ou comportamento conforme a noite avança.

## Janela de tempo do encontro

O encontro acontece durante a madrugada.

Horário base sugerido:

- Início: 23h ou 00h.
- Fim: 04h.

Quando o horário final chegar, o evento acaba e todos vão embora.

A passagem de tempo deve ser usada como ferramenta de design para alterar o conteúdo disponível no encontro.

## Estrutura de fases da noite

A noite pode ser dividida em blocos de rotina:

### Bloco 1 — 23h/00h até 01h

Clima de chegada.

Possibilidades:

- Poucos grupos presentes.
- Pessoas chegando aos poucos.
- Três grupos principais de NPCs.
- Conversas introdutórias.
- Jogador conhece o local e os participantes.

### Bloco 2 — 01h até 03h

Pico do encontro.

Possibilidades:

- Mais pessoas aparecem.
- Alguns grupos mudam de lugar.
- Novos carros ficam disponíveis para observar.
- Conversas diferentes aparecem.
- Novos desafios ou provocações podem surgir futuramente.
- O evento passa a parecer mais cheio e movimentado.

### Bloco 3 — 03h até 04h

Final da noite.

Possibilidades:

- Algumas pessoas vão embora.
- Grupos diminuem.
- Conversas ficam mais diretas ou tensas.
- Últimas oportunidades de interação.
- Preparação para encerrar o encontro.

## Sistema de rotinas

Para o encontro parecer vivo, será necessário um sistema de rotinas para NPCs e grupos.

A rotina pode controlar:

- Presença ou ausência de NPCs em determinado horário.
- Posição dos NPCs no cenário.
- Grupo ao qual o NPC pertence.
- Conversas disponíveis.
- Animações ou estados visuais.
- Interações disponíveis.
- Carros associados ao NPC ou grupo.

A ideia é que o mesmo cenário possa ter comportamentos diferentes dependendo do horário da noite.

Exemplo conceitual:

```txt
23h-01h:
  - Grupo Oficina A perto dos carros vermelhos.
  - Piloto X conversando perto do capô aberto.
  - Poucos NPCs no estacionamento.

01h-03h:
  - Grupo Oficina B chega.
  - Piloto X muda para perto da entrada.
  - Mais NPCs aparecem perto dos carros.

03h-04h:
  - Grupo Oficina A vai embora.
  - Piloto Y fica disponível para última conversa.
  - Ambiente começa a esvaziar.
```

## Interações planejadas

Interações iniciais possíveis:

- Conversar com NPCs.
- Inspecionar carros.
- Interagir com objetos do cenário.
- Fumar cigarro.

### Fumar cigarro

O cigarro será uma interação do encontro e pode dar benefícios para a corrida.

Ainda precisa ser definido:

- Qual benefício será aplicado.
- Duração do efeito.
- Se existe algum custo, risco ou limite.
- Como o efeito será comunicado ao jogador.

## Carros no encontro

Os carros estacionados são parte importante da ambientação.

Eles podem servir para:

- Mostrar o nível dos pilotos presentes.
- Dar personalidade para grupos/oficinas.
- Criar pontos de interesse no mapa.
- Futuramente indicar possíveis oponentes de corrida.

No primeiro momento, o jogador deve poder ao menos olhar/inspecionar carros, mesmo que a interação seja simples.

## Grupos de pessoas

Os grupos ajudam a dar identidade ao meet.

Tipos possíveis:

- Grupo de uma oficina.
- Grupo de amigos de um piloto.
- Grupo de curiosos.
- Grupo de apostadores.
- Grupo de mecânicos.
- Grupo de pilotos rivais.

Cada grupo pode ter:

- Local preferido no cenário.
- Carros associados.
- NPCs principais.
- Conversas específicas.
- Rotinas por horário.

## Direção técnica inicial

A implementação provavelmente vai evoluir o modo debug atual para, no futuro, virar uma espécie de **editor de cenário**.

Por enquanto, isso é apenas uma direção técnica. Não vamos alterar o modo debug ainda.

Ideia futura:

- Usar o modo debug para posicionar carros, NPCs e objetos.
- Salvar pontos de spawn/interação em arquivos de dados.
- Criar zonas de interação.
- Definir grupos e rotinas visualmente.
- Facilitar iteração no layout do encontro.

## Possível modelo de dados

Uma estrutura futura pode ter arquivos de configuração para:

- Cenário do meet.
- Pontos de spawn.
- Carros estacionados.
- NPCs.
- Grupos.
- Rotinas por horário.
- Conversas.
- Interações.

Exemplo conceitual:

```txt
night_meet:
  id: downtown_garage
  start_time: 23:00
  end_time: 04:00

  groups:
    - id: oficina_a
      name: Oficina A
      cars: [...]
      npcs: [...]
      routines:
        - from: 23:00
          to: 01:00
          area: north_parking
        - from: 01:00
          to: 03:00
          area: main_lot

  interactions:
    - type: inspect_car
    - type: dialogue
    - type: smoke
```

O formato final ainda deve ser decidido de acordo com os padrões atuais do projeto.

## Sistemas necessários futuramente

Para os Night Meets ficarem completos, provavelmente precisaremos dos seguintes sistemas:

1. **Modo a pé do jogador**
   - Controle básico andando pelo cenário.
   - Câmera apropriada para exploração.
   - Colisão com o ambiente.

2. **Sistema de interação**
   - Detectar objetos/NPCs próximos.
   - Mostrar prompt de interação.
   - Executar ações como conversar, inspecionar ou fumar.

3. **Sistema de NPCs**
   - Spawns.
   - Estados simples.
   - Associação com grupos.
   - Presença por horário.

4. **Sistema de rotinas por horário**
   - Trocar disposição do meet conforme a noite avança.
   - Ativar/desativar NPCs, carros e conversas.

5. **Sistema de diálogo**
   - Conversas simples.
   - Variações por horário, grupo ou progresso.

6. **Sistema de cenário/editoração**
   - Evolução futura do modo debug.
   - Posicionar entidades e salvar dados.

7. **Integração futura com corridas/apostas**
   - Escolher oponente.
   - Definir aposta.
   - Iniciar corrida.
   - Resolver consequência.

## Plano de implementação sugerido

### Etapa 1 — Documento e design

- Documentar a visão da feature.
- Definir escopo inicial.
- Levantar sistemas necessários.
- Decidir como os dados serão organizados.

### Etapa 2 — Protótipo do cenário

- Criar uma cena simples de garagem/estacionamento.
- Permitir o jogador andar a pé.
- Colocar carros e NPCs estáticos.
- Criar pontos básicos de interação.

### Etapa 3 — Interações básicas

- Conversar com NPCs.
- Inspecionar carros.
- Interagir com objeto de cigarro.
- Mostrar prompts de interação.

### Etapa 4 — Rotinas por horário

- Implementar relógio do meet.
- Dividir a noite em blocos.
- Alterar NPCs, carros e grupos conforme horário.

### Etapa 5 — Ferramentas de edição

- Evoluir o modo debug para auxiliar na criação dos meets.
- Permitir posicionar entidades.
- Exportar/salvar configuração do cenário.

### Etapa 6 — Integração futura com apostas

- Conectar NPCs/pilotos a desafios de corrida.
- Permitir escolher oponente.
- Definir aposta.
- Sair do meet para iniciar corrida.

## Perguntas em aberto

- O jogador chega ao meet dirigindo ou já inicia a pé?
- O meet será uma cena separada ou parte do mesmo mapa da corrida?
- O tempo passa automaticamente ou por eventos?
- O jogador pode sair antes das 04h?
- O cigarro terá benefício sem penalidade ou terá algum trade-off?
- Como serão representadas as conversas inicialmente?
- Os NPCs terão movimentação real ou apenas mudanças de posição por bloco de horário?
- Quantos carros e NPCs o primeiro protótipo deve ter?

## Decisão atual

Por enquanto, vamos apenas manter este planejamento documentado.

A implementação deve começar depois, provavelmente com um protótipo simples de cenário e interação, sem mexer ainda no sistema de apostas ou transformar o modo debug em editor completo.
