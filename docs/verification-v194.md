# Volvy GBA v1.9.4 — verification status

- Base: recovered v1.9 → v1.9.1 → v1.9.2 → v1.9.3 → v1.9.4.
- BG1 scene builder writes into persistent OAM-independent tile upload queue.
- 540 cache churn scroll/warp cases agree with independent first-fit allocator.
- Portable host test suite: PASS; ARMv4T/Thumb test-stub build: PASS.
- Clang/LLD bare-metal ROM: PASS (GBA header and ELF structural checks only).
- mGBA execution: not checked; physical GBA: not checked.
- No changes to source-converted art, gameplay mechanics, placements or levels.

Regression entry points: `sh tests/run_host_tests.sh`,
`sh tests/run_arm_codegen.sh`, `sh tests/run_experimental_clang.sh`.
