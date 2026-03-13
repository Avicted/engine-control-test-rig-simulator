#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INTERACTIVE=1
SKIP_VISUALIZER=0

if [[ ! -t 0 ]]; then
	INTERACTIVE=0
fi

if [[ -t 1 ]] && command -v tput >/dev/null 2>&1; then
	BOLD="$(tput bold)"
	DIM="$(tput dim)"
	RED="$(tput setaf 1)"
	GREEN="$(tput setaf 2)"
	YELLOW="$(tput setaf 3)"
	BLUE="$(tput setaf 4)"
	RESET="$(tput sgr0)"
else
	BOLD=""
	DIM=""
	RED=""
	GREEN=""
	YELLOW=""
	BLUE=""
	RESET=""
fi

usage() {
	cat <<'EOF'
Usage: ./demo.sh [options]

Options:
  --auto             Run without waiting for Enter between stages.
  --skip-visualizer  Keep the demo terminal-only.
  --help             Show this help message.
EOF
}

for arg in "$@"; do
	case "$arg" in
		--auto)
			INTERACTIVE=0
			;;
		--skip-visualizer)
			SKIP_VISUALIZER=1
			;;
		--help)
			usage
			exit 0
			;;
		*)
			printf '%sUnknown option:%s %s\n' "$RED" "$RESET" "$arg" >&2
			usage >&2
			exit 1
			;;
	esac
done

headline() {
	printf '\n%s%s%s\n' "$BOLD" "$1" "$RESET"
}

subhead() {
	printf '%s%s%s\n' "$BLUE" "$1" "$RESET"
}

note() {
	printf '%s%s%s\n' "$DIM" "$1" "$RESET"
}

success() {
	printf '%s%s%s\n' "$GREEN" "$1" "$RESET"
}

warn() {
	printf '%s%s%s\n' "$YELLOW" "$1" "$RESET"
}

die() {
	printf '%s%s%s\n' "$RED" "$1" "$RESET" >&2
	exit 1
}

pause_for_stage() {
	local prompt_text=${1:-"Press Enter to continue..."}
	if [[ "$INTERACTIVE" -eq 1 ]]; then
		printf '\n%s' "$prompt_text"
		read -r _
	fi
}

run_cmd() {
	local description=$1
	shift
	subhead "$description"
	printf '%s$%s' "$DIM" "$RESET"
	printf ' %q' "$@"
	printf '\n'
	"$@"
}

run_optional_cmd() {
	local description=$1
	shift
	subhead "$description"
	printf '%s$%s' "$DIM" "$RESET"
	printf ' %q' "$@"
	printf '\n'
	if ! "$@"; then
		warn "Optional step failed: $description"
		return 1
	fi
	return 0
}

show_intro() {
	headline "Engine Control Test Rig Demo"
	note "Audience: tech recruiters"
	note "Flow: rebuild, quality snapshot, healthy scenario, shutdown scenario, optional visualizer"
	note "Tip: use --auto if you want the whole sequence to run without pauses."
}

show_quality_signals() {
	headline "Engineering Snapshot"
	cat <<'EOF'
- 255 unit tests in the suite
- 100% line coverage on the unit-tested modules
- 10 built-in validation scenarios with requirement-tagged output
- Deterministic replay verified in CI through hash-matched runs
EOF
	note "Live proof next: the built-in scenario sweep."
	run_cmd "Run the built-in validation suite" make --no-print-directory run-all-short
	success "Takeaway: the simulator already exposes requirement-tagged validation output from the CLI."
}

show_normal_scenario() {
	headline "Healthy Baseline"
	note "This keeps the path simple: one scripted scenario, one clear OK result."
	run_cmd "Run the normal operation scenario" ./build/testrig --script scenarios/normal_operation.txt
	success "Takeaway: healthy inputs stay in OK, which gives a clean baseline before fault injection."
}

show_shutdown_scenario() {
	headline "Fault Escalation"
	note "This scenario crosses the temperature persistence window and drives the controller to SHUTDOWN."
	run_cmd "Run the overheat persistence scenario" ./build/testrig --script scenarios/overheat_persistent_shutdown.txt
	note "Optional machine-readable view:"
	run_cmd "Show the same scenario as deterministic JSON" ./build/testrig --script scenarios/overheat_persistent_shutdown.txt --json
	success "Takeaway: the same deterministic core supports both human-readable and tool-friendly output."
}

can_launch_visualizer() {
	if [[ "$SKIP_VISUALIZER" -eq 1 ]]; then
		return 1
	fi

	if ! command -v python3 >/dev/null 2>&1; then
		warn "Skipping visualizer: python3 is not available."
		return 1
	fi

	if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
		warn "Skipping visualizer: no graphical display detected."
		return 1
	fi

	return 0
}

ask_visualizer() {
	if [[ "$INTERACTIVE" -ne 1 ]]; then
		return 0
	fi

	printf '\nLaunch the optional visualizer? [Y/n] '
	read -r reply
	case "$reply" in
		""|[Yy]|[Yy][Ee][Ss])
			return 0
			;;
		*)
			warn "Skipping visualizer at presenter request."
			return 1
			;;
	esac
}

show_visualizer() {
	if ! can_launch_visualizer; then
		return 0
	fi

	if ! ask_visualizer; then
		return 0
	fi

	headline "Optional Visualizer"
	note "This opens the read-only Raylib dashboard backed by generated JSON. Close the window to return to the shell."
	if ! run_optional_cmd "Generate the visualization bundle" make --no-print-directory generate-visualization-json; then
		warn "Continuing without the visualizer."
		return 0
	fi
	if ! run_optional_cmd "Build the visualizer" make --no-print-directory visualizer; then
		warn "Continuing without the visualizer."
		return 0
	fi
	if ! run_optional_cmd "Launch the visualizer" make --no-print-directory run-visualizer JSON=visualization/scenarios.json; then
		warn "The rest of the demo is still complete without the visualizer."
		return 0
	fi
	success "Takeaway: the simulator output can drive a polished playback UI without direct coupling to internals."
}

show_wrap_up() {
	headline "Wrap-Up"
	cat <<'EOF'
- Rebuild is one command: make
- Validation is requirement-tagged and deterministic
- Scripted scenarios make fault injection easy to explain live
- Visualization is optional and read-only, built from JSON output
EOF
}

require_repo_file() {
	local path=$1
	[[ -e "$path" ]] || die "Missing required file: $path"
}

require_command() {
	local name=$1
	command -v "$name" >/dev/null 2>&1 || die "Required command not found: $name"
}

main() {
	cd "$SCRIPT_DIR"
	require_command make
	require_repo_file "Makefile"
	require_repo_file "scenarios/normal_operation.txt"
	require_repo_file "scenarios/overheat_persistent_shutdown.txt"

	show_intro
	pause_for_stage "Press Enter to rebuild the simulator..."
	run_cmd "Rebuild the simulator" make --no-print-directory
	[[ -x ./build/testrig ]] || die "Build finished but ./build/testrig is missing or not executable."
	success "Build complete."

	pause_for_stage "Press Enter for the engineering snapshot..."
	show_quality_signals

	pause_for_stage "Press Enter for the healthy baseline scenario..."
	show_normal_scenario

	pause_for_stage "Press Enter for the fault escalation scenario..."
	show_shutdown_scenario

	show_visualizer
	show_wrap_up
}

main