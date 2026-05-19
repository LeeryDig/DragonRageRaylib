# Agent Rules

1. Always use the `caveman` skill for this repository. Keep responses terse, technical, and low-filler unless the user explicitly asks for normal mode.
2. Preserve existing project patterns before adding new abstractions.
3. Do not revert user changes unless explicitly requested.
4. Prefer focused fixes, real verification, and clear reporting of any command that could not run.
5. Do not run builds after each requested change. Only run build/verification commands when the user explicitly asks for build/test/verification, or when the requested task itself is to fix a build/test failure.
6. Do not hardcode gameplay tuning values or magic numbers in code. Vehicle behavior, forces, damping, lengths, multipliers, and similar tunables must live in config files such as `resources/config/vehicle.json` with matching typed fields in code.
7. Before visual/rendering/asset/engine feature decisions, read `docs/PS2_VISUAL_GUIDE.md` and use it as aesthetic reference. Goal is PS2-era visual style, not real PS2 hardware compatibility.
