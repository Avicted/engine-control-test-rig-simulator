# Release Artifacts

This project publishes runnable Linux and Win64 release bundles for both the simulator and the Raylib visualizer.

## Published assets

Tagging the repository with `v*` drives `.github/workflows/release.yml`, which builds and uploads:

- `engine-control-test-rig-simulator-linux-x64.tar.gz`
- `engine-control-test-rig-simulator-win64.zip`

Each archive contains:

- `testrig` or `testrig.exe`
- `visualizer` or `visualizer.exe`
- `calibration.json`
- `scenarios/`
- `schema/engine_test_rig.schema.json`
- `tests/integration/invalid_script.txt`
- `tools/release_audit.py`
- `tools/validate_json_contract.py`
- `visualization/scenarios.json`
- `visualization/PxPlus_IBM_EGA_8x14.ttf`

Platform-specific runtime packaging:

- Linux bundles discovered shared libraries under `lib/` and provides `run-testrig.sh` plus `run-visualizer.sh` wrapper scripts that set `LD_LIBRARY_PATH`.
- Win64 bundles the required non-system MinGW runtime DLLs next to the `.exe` files.

## Shipped audit workflow

Every bundle includes a portable black-box audit entry point:

```bash
python3 tools/release_audit.py
```

The audit script verifies the packaged simulator and runtime data by:

- running the built-in validation suite in console and JSON modes
- validating JSON output against the shipped schema
- checking the calibration-file path
- checking the negative parser path with the shipped invalid-script fixture
- regenerating the visualization bundle from the packaged simulator outputs and comparing it to the shipped `visualization/scenarios.json`
- smoke-launching the visualizer unless `--skip-visualizer` is used

Windows invocation:

```powershell
py -3 tools\release_audit.py
```

or:

```powershell
python tools\release_audit.py
```

The GitHub Release workflow runs the packaged simulator audit in both the Linux and Win64 jobs before uploading artifacts. Those CI audits use `--skip-visualizer` because hosted runners are headless.

## Reviewer quick start

The released binaries are command-line tools. Running them with no arguments prints usage; reviewers should use the exact commands below.

Linux bundle review:

```bash
cd engine-control-test-rig-simulator-linux-x64

./run-testrig.sh --version
./run-testrig.sh --run-all
./run-testrig.sh --script scenarios/normal_operation.txt --json
./run-visualizer.sh visualization/scenarios.json
python3 tools/release_audit.py --bundle-dir .
```

Win64 bundle review on Windows:

```powershell
cd engine-control-test-rig-simulator-win64

.\testrig.exe --version
.\testrig.exe --run-all
.\testrig.exe --script scenarios\normal_operation.txt --json
.\visualizer.exe visualization\scenarios.json
py -3 tools\release_audit.py
```

Win64 bundle review on Linux with Wine:

```bash
cd engine-control-test-rig-simulator-win64

wine ./testrig.exe --version
wine ./testrig.exe --run-all
wine ./testrig.exe --script scenarios/normal_operation.txt --json
wine ./visualizer.exe visualization/scenarios.json
python3 tools/release_audit.py --bundle-dir . --command-prefix wine --skip-visualizer --skip-visualization-regeneration
```

Reviewer notes:

- The Linux bundle uses `run-testrig.sh` and `run-visualizer.sh` as the supported entry points because they set `LD_LIBRARY_PATH` for the bundled runtime library directory.
- The Win64 bundle does not include `run-*.sh` wrappers. On Windows, launch the `.exe` files directly; on Linux, launch them with `wine`.
- The visualizer always needs at least one JSON input path, and the shipped bundle is `visualization/scenarios.json`.

## Local artifact testing

Local artifact testing exercises the packaged bundles, not just the build tree:

- Linux: builds the tarball, bundles shared libraries, unpacks the archive, and runs the shipped audit.
- Win64 on Linux: cross-builds Raylib and the project, creates the zip, unpacks it, and runs the shipped audit against the packaged `.exe` files via Wine.

The local Wine-based audit skips the visualization-bundle regeneration comparison because repeated Wine process startup makes that specific check disproportionately slow. The rest of the simulator audit still runs, and native release audits can still execute the full regeneration path.

Dry-run commands:

```bash
make test-release-linux RELEASE_PLATFORM=linux-x64 RELEASE_ARCHIVE=./dist/engine-control-test-rig-simulator-linux-x64.tar.gz CC=gcc PYTHON=python3
make test-release-win64-local RELEASE_PLATFORM=win64 RELEASE_ARCHIVE=./dist/engine-control-test-rig-simulator-win64.zip PYTHON=python3
```

Prerequisites for local testing on Linux:

- Native Linux path: `gcc`, `make`, `python3`, `raylib`, `ldd`, and a working GUI session for the visualizer smoke launch.
- Win64-on-Linux path: `x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`, `cmake`, `git`, and `wine`.

## Pre-tag checklist

Use this sequence before pushing a release tag:

1. Run the local Linux artifact test.

```bash
make test-release-linux RELEASE_PLATFORM=linux-x64 RELEASE_ARCHIVE=./dist/engine-control-test-rig-simulator-linux-x64.tar.gz CC=gcc PYTHON=python3
```

2. Run the local Win64 artifact test under Wine.

```bash
make test-release-win64-local RELEASE_PLATFORM=win64 RELEASE_ARCHIVE=./dist/engine-control-test-rig-simulator-win64.zip PYTHON=python3
```

3. Check the working tree and review the release-related diff.

```bash
git status
git diff --stat
```

4. Commit the release changes.

```bash
git add .
git commit -m "Add audited cross-platform release artifacts"
```

5. Create and push the release tag.

```bash
git tag v0.1.0
git push origin HEAD
git push origin v0.1.0
```

6. Verify the GitHub Release workflow succeeds and that both uploaded artifacts are present on the GitHub release page.

7. As a final spot-check, download each published archive, unpack it, and run the shipped audit from the bundle itself.

## VS Code tasks

Workspace tasks in `.vscode/tasks.json` support the local release workflow:

- `release: test linux artifact`
- `release: test win64 artifact with wine`
- `release: package linux artifact`
- `release: package win64 artifact`
- `release: generate visualization bundle`

Run them from Terminal -> Run Task in VS Code.