# Física de Superfícies de Pista

## Implementado

`COL_Road*` e `COL_Ramp*` do `.glb` → `LevelRoadSurface` (vértices world-space + índices).

- Raycast de suspensão: `GetRayCollisionTriangle()` por triângulo real.
- Contato do corpo do carro: pontos do box do veículo amostram superfície, calcula normal do triângulo, aplica impulso/correção de penetração.
- Outros `COL_*` (paredes, obstáculos) continuam como box colliders (OBB).
- Debug draw: arestas dos triângulos reais.

## Roadmap

**Shape formal de superfície**
```cpp
class RoadSurfaceShape : public Shape {
    std::vector<Triangle> triangles;
};
```
Integrar no `PhysicsWorld` como static body com narrowphase para raycast e box vs superfície.

**Centralizar raycast na física**
```cpp
bool PhysicsWorld::Raycast(const Ray& ray, CollisionMask mask, RaycastHit& hit);
```
Remove dependência do veículo no tipo de collider.

**Otimização espacial**

Grid/BVH simples quando houver muitas surfaces. Raycast testa só triângulos próximos ao ponto de consulta.

**Materiais de pista por nome de node**

```
COL_Road_Asphalt_001  → grip alto, drag baixo
COL_Road_Dirt_001     → grip baixo, drag alto
COL_Ramp_Metal_001    → superfície metálica
```

Parâmetros por material: `friction`, `grip`, `drag`, som, partículas.
