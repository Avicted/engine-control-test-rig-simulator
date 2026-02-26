# Determinism Guarantee

## Objective

Simulation runs are expected to produce **byte-identical output** for the
same inputs when executed in a controlled environment (same compiler/toolchain,
locale, and simulator revision).

## How Determinism Is Achieved

| Control                               | Implementation                               |
| ------------------------------------- | -------------------------------------------- |
| No dynamic allocation                 | All arrays are statically sized               |
| No recursion                          | Deterministic call depth                      |
| Floating-point variance bounded       | IEEE 754, pinned compiler flags, no `rand()`   |
| No time-of-day or wall-clock usage    | Tick counter is the sole time source          |
| No I/O-dependent ordering             | FIFO queues with bounded capacity             |
| Stable data layout contracts          | `_Static_assert` enforces ABI/layout contracts |
| Single-threaded execution only        | No concurrency or shared mutable state        |

## CI Enforcement

The `determinism-check` Makefile target runs the simulator twice on the
same input set and compares SHA-256 hashes of the full JSON output.
If the hashes differ, CI fails immediately.

```
make determinism-check
```

## Replay Test

The script `tools/determinism_check.sh` implements the hash comparison.
It can also be run standalone:

```
sh tools/determinism_check.sh ./build/testrig
```

## Known Non-Determinism Risks

- **Compiler version changes** may alter floating-point rounding.
  Mitigated by pinning the compiler in CI.
- **`printf` locale** may affect decimal formatting.
  Mitigated by using the C locale (default).
