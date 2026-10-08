# Phase 1 working files and evidence

Baseline commit: `3811ad0a87bd490e45099c3cb179ec33caf46cb5`.
The upstream files remain in place. The written report is maintained in HackMD;
its published revision link must be recorded with the eventual submitted tag.

This is a working record, not a declaration that every assignment requirement is
complete. Assembly authorship and measured iterative assembly refinement remain
unresolved. See [AI_USAGE.md](AI_USAGE.md) before selecting a submission snapshot.

## File locations

| Location | Contents |
|---|---|
| [rv32i/minirubik_rv32i.s](rv32i/minirubik_rv32i.s) | Renderer-excluded Ripes source |
| [rv32i/minirubik_rv32i_led.s](rv32i/minirubik_rv32i_led.s) | Ripes source with LED renderer |
| [rv32i/core.s](rv32i/core.s), [rv32i/renderer.s](rv32i/renderer.s), [rv32i/build.py](rv32i/build.py) | Shared templates and host build script for both sources |
| [test_records/c](test_records/c) | Original and Step 1/2/3 C versions, headers and validation harnesses |
| [test_records/c/host-validation/original-c-full-report.txt](test_records/c/host-validation/original-c-full-report.txt) | Original C full-domain validation |
| [test_records/c/host-validation-step3/step3-full-report.txt](test_records/c/host-validation-step3/step3-full-report.txt) | Step 3 C full-domain validation |
| [test_records/evidence/ripes-tests](test_records/evidence/ripes-tests) | Three-case records, single-input records, LED settings and screenshots |
| [test_records/evidence/pipeline](test_records/evidence/pipeline) | Pipeline observations and memory/register screenshots |
| [test_records/evidence/hard-ripes-full](test_records/evidence/hard-ripes-full) | Frozen tested source, configuration, all 2,644 case records and summary |
| [test_records/gcc-reference](test_records/gcc-reference) | GCC reference sources, build script, ELF, section sizes and disassembly |
| [test_records/evidence/gcc-reference](test_records/evidence/gcc-reference) | Student-recorded reference measurement |

The historical reports retain their original absolute source paths and timings.
The C versions are preserved artifacts, not retrospectively dated Git commits.
The root `solver.c` is the upstream baseline. The student-supplied C source with
Chinese comments is `test_records/c/solver_original.c`; the final C refinement
is `test_records/c/solver_rv32i_step3.c`. These are distinct from the root file.
Assistant probe records and local `.exe`/`.o` intermediates are omitted from this
collection. Host BFS oracle binaries are validation inputs; they are not embedded
in the RV32I program.

## Generate the Ripes sources

With Python installed, from the repository root:

```powershell
python rv32i/build.py
```

The two standalone sources are emitted into `rv32i/`. This script combines the
shared core, renderer, PDBs and host-generated transition tables. It does not run
the target solver or produce a new measurement. `RUN_TESTS` and `input_state`
are defined in the sources; the frozen source under `hard-ripes-full` is the
artifact associated with the exhaustive target evidence.

For GUI visualization, load the LED source in Ripes, select RV32I without
extensions, and configure LED Matrix Width 35 and Height 25. The saved pipeline
observations use RV32_5S. Performance measurements use RV32_ISS with the renderer
excluded. The pinned Ripes build is `v2.2.6-106-g5b8a616`.

## Rebuild host validation on Windows

The full-domain harnesses use `windows.h`. With a Windows native C compiler
available in PATH, run from `test_records/c`:

```powershell
gcc -O2 test_step3_three_cases.c -o test_step3_three_cases.exe
./test_step3_three_cases.exe
gcc -O2 validate_step3_host.c -o validate_step3_host.exe
./validate_step3_host.exe --smoke
```

Use `--full` for all 3,674,160 states. The analogous original-version harness is
`validate_original_host.c`. Run a new measurement separately from preserved
reports, rather than replacing the historical report with a later run.

## Rebuild the GCC reference

The copied build script accepts the toolchain directory explicitly. From the
repository root, using the original local toolchain installation:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./test_records/gcc-reference/build_reference.ps1 -ToolchainBin 'C:\Users\USER\Documents\Codex\2026-10-06\new-chat-2\work\riscv-gcc\bin'
```

Compilation uses `-O2 -march=rv32i -mabi=ilp32 -S`, followed by GNU assembly and
linking without the standard C library. This avoids requiring an installed
RV32I C-runtime multilib. The script refreshes its ELF and build reports; these
outputs do not constitute a new Ripes execution measurement.

## Target validation runner

[test_records/run_hard_ripes.py](test_records/run_hard_ripes.py) is the preserved
host runner. Its `RIPES` constant records the local simulator installation and
must be adjusted on another machine. It uses the host oracle and frozen source,
and independently replays the returned paths. Existing results are resumed;
use a separate working copy for a new batch so the historical timing summary
is preserved. The completed batch is documented in its `configuration.json`,
`results.csv`, per-input records and `summary.txt`.
