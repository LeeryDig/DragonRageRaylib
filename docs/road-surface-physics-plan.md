# Plano para evoluir física de pistas por superfície

Status atual: `COL_Road*` e `COL_Ramp*` são lidos do `.glb` como geometria real de triângulos. O loader transforma os vértices para world space, a suspensão usa raycast contra esses triângulos e o contato do corpo do carro também usa as superfícies da pista.

## Objetivo

Evoluir a física para tratar superfícies de pista como um tipo formal de collider, em vez de lógica especial dentro do gameplay.

## Estado atual

- `COL_Road*` / `COL_Ramp*` viram `LevelRoadSurface`.
- Cada surface guarda:
  - vértices em world space;
  - índices;
  - posição/rotação/tamanho para debug/fallback.
- Raycast da suspensão usa `GetRayCollisionTriangle()` em cada triângulo.
- Contato do corpo do carro usa os vértices do box do veículo, amostra a superfície da pista, calcula normal do triângulo e aplica impulso/correção de penetração nessa normal.
- Outros `COL_*`, como paredes/obstáculos, continuam como box colliders.

## Implementado no gameplay

- Raycast de suspensão contra triângulos reais de `COL_Road*`.
- Debug draw usando arestas dos triângulos reais.
- Contato do corpo do carro contra pista por vértices do collider do veículo.
- Correção de penetração usando normal da superfície.

## Próximos passos

### 1. Criar shape de superfície

Adicionar algo como:

```cpp
class TriangleSurfaceShape : public Shape {
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
};
```

Ou uma versão mais simples:

```cpp
class RoadSurfaceShape : public Shape {
    std::vector<Triangle> triangles;
};
```

### 2. Integrar com PhysicsWorld

Permitir que o `PhysicsWorld` tenha static bodies com surface shape.

Inicialmente, o narrowphase precisa suportar:

- raycast contra superfície;
- body box vs superfície;
- trigger opcional no futuro.

### 3. Centralizar raycast

Hoje o raycast de roda está no gameplay.

Mover para API da física:

```cpp
bool PhysicsWorld::Raycast(const Ray& ray, CollisionMask mask, RaycastHit& hit);
```

Assim o veículo não precisa saber se está colidindo contra box, surface ou mesh.

### 4. Contato box vs superfície

Para contato do carro com pista:

- pegar pontos de contato do box do carro;
- testar projeção contra triângulos da superfície;
- calcular penetração e normal;
- gerar contato com normal da face.

### 5. Otimização espacial

Quando houver muitas surfaces:

- criar grid/BVH simples;
- raycast só testa triângulos próximos;
- separar surfaces por setor da pista.

### 6. Materiais de pista

Permitir metadata por nome:

```txt
COL_Road_Asphalt_001
COL_Road_Dirt_001
COL_Ramp_Metal_001
```

E associar materiais:

- friction;
- grip;
- drag;
- som/partículas.

## Resultado esperado

A pista deixa de ser uma lógica especial do `main.cpp` e passa a ser parte da física formal do jogo.

Fluxo final desejado:

```txt
Blender COL_Road
  ↓
LevelLoader lê triângulos
  ↓
PhysicsWorld cria RoadSurfaceShape
  ↓
Veículo faz raycast genérico
  ↓
Física resolve contato com superfície
```
