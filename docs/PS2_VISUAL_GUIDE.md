# PS2 Visual Guide

Goal: use PlayStation 2 as visual reference, not hardware target. We do **not** need engine/features/assets to run on real PS2. Constraints below guide art direction, budgets, and rendering choices so final image feels PS2-era.

## Core Rule

Every visual/render/asset decision should answer:

> Does this help game look like a PlayStation 2-era game, or does it push too far into modern rendering?

Prefer PS2-like look over technical accuracy when needed.

## Reference Hardware Context

Use these numbers as flavor/budget reference, not strict limits:

- CPU: Emotion Engine, ~294 MHz
- Main RAM: 32 MB
- GPU: Graphics Synthesizer, ~147 MHz
- VRAM: 4 MB eDRAM
- Common output: 512x448, 640x448, 640x480
- Common aspect: 4:3
- No modern programmable shader pipeline
- Strong fillrate for era, but tight memory/VRAM and streaming limits

## Resolution / Presentation

Recommended defaults:

- Internal render resolution: 640x448 or 640x480
- Aspect: 4:3 first; widescreen optional/anamorphic-style
- Upscale with controlled softness or nearest/linear hybrid
- Visible aliasing acceptable
- Optional post effects:
  - dithering
  - color banding
  - slight blur/soft upscale
  - interlace/CRT-style pass if tasteful

Avoid pristine modern image unless explicitly desired.

## Texture Rules

Texture sizes should feel constrained:

- Tiny props/details: 32x32 or 64x64
- Common props/world tiles: 64x64 or 128x128
- Important props/characters: 128x128 or 256x256
- 512x512: rare exception, not default

Preferred techniques:

- Tiling textures
- Small atlases
- Palette-like color discipline
- Hand-painted diffuse detail
- Vertex color tint/variation
- Low-frequency baked shading

Avoid by default:

- Large 1K/2K/4K textures
- Modern high-detail normal maps
- Full PBR texture sets
- Ultra-crisp surface detail

## Geometry Budgets

Use these as visual targets, not hard caps:

- Main character: ~1k–5k triangles
- Important enemy/NPC: ~500–2k triangles
- Small prop: ~50–500 triangles
- Medium prop: ~500–1500 triangles
- Visible scene: think tens of thousands of triangles, not millions

Style goals:

- Clear angular silhouettes
- Limited edge loops
- Selective smoothing groups/normals
- Geometry detail where silhouette matters
- Texture/vertex color used for smaller detail

## Materials

Default material model should be PS2-like, not PBR.

Preferred material inputs:

- Diffuse/albedo texture
- Vertex color
- Baked lighting/light factor
- Optional fake specular
- Optional fake environment map/reflection
- Optional emissive mask for stylized effects

Avoid default dependence on:

- Physically-based roughness/metalness workflow
- Heavy normal mapping
- Screen-space modern realism
- HDR-style bloom as baseline

## Lighting

Preferred lighting stack:

- Baked ambient/shading
- Vertex lighting
- Low-resolution lightmaps where useful
- Few dynamic lights
- Fake/specular highlights where needed
- Blob/projected/simple shadows
- Fog for depth and draw-distance control

Avoid:

- Fully dynamic realistic lighting as default
- Complex shadow cascades as core look
- SSAO/SSR/global illumination that makes image too modern

## Fog / Draw Distance

Fog is core PS2-era tool.

Use fog to:

- Hide far clip / asset density limits
- Add atmosphere
- Simplify open scenes
- Create era-authentic depth

LOD popping is acceptable if not ugly. Smooth modern infinite draw distance is not visual target.

## Animation

Reference targets:

- Main character skeleton: ~20–40 bones
- Simpler NPC: ~10–25 bones
- Facial animation: simple bones, jaw, eye controls, or texture swaps

Style:

- Clear keyed poses
- Limited blend complexity
- Slight mechanical/gamey transitions acceptable
- Avoid overly fluid modern mocap feel unless intentional

## Level / World Design

PS2-era feel benefits from:

- Segmented spaces
- Occlusion through corridors, turns, gates, cliffs, buildings
- Asset reuse with tint/scale/material variation
- Loading/streaming hidden by layout if needed
- Set dressing with low-poly repeated props

Avoid huge open worlds with modern density unless visually filtered through PS2 constraints.

## Effects

Preferred effects:

- Billboard particles
- Simple additive sprites
- Low-res smoke/fire textures
- Screen-space flashes
- Fake trails
- Simple water/reflection tricks

Avoid default:

- Physically complex particles
- Volumetrics
- Modern cinematic bloom/lens dirt
- High-resolution fluid/simulation effects

## Raylib/OpenGL Implementation Direction

Good defaults for this engine:

- Render scene to low-resolution framebuffer, then upscale
- Keep material system simple/diffuse-first
- Add optional dither/banding post-process
- Add configurable fog globally/per-scene
- Support vertex colors in meshes
- Prefer baked/author-driven lighting data
- Keep fake specular/envmap as explicit stylized options
- Make modern features opt-in, not baseline

## Asset Checklist

Before adding/importing asset:

- Texture size fits PS2-like target?
- Polycount visually justified?
- Reads well at 640x448/480?
- Uses diffuse/vertex/baked detail instead of modern PBR dependence?
- Has reuse/tile/atlas opportunity?
- Silhouette strong enough despite low detail?

## Feature Checklist

Before implementing visual feature:

- Does feature support PS2-like image?
- Can it be tuned down/off?
- Does default look become too modern?
- Can same result be faked cheaper/simpler?
- Does it preserve low-res, constrained, stylized feel?

## Important Clarification

This guide is aesthetic reference, not a promise of real PS2 compatibility. We can use modern CPU/GPU features internally when they help development, stability, or productivity. Final output should still respect PS2-era visual language.
