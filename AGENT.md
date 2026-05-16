# Agent Rules

1. Always use the `caveman` skill for this repository. Keep responses terse, technical, and low-filler unless the user explicitly asks for normal mode.
2. Preserve existing project patterns before adding new abstractions.
3. Do not revert user changes unless explicitly requested.
4. Prefer focused fixes, real verification, and clear reporting of any command that could not run.
5. Do not run builds after each requested change unless the user explicitly asks for a build/verification run.
6. Do not hardcode gameplay tuning values or magic numbers in code. Vehicle behavior, forces, damping, lengths, multipliers, and similar tunables must live in config files such as `resources/config/vehicle.json` with matching typed fields in code.
