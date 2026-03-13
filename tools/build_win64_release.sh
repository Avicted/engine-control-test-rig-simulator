#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
RAYLIB_SRC_DIR=${RAYLIB_SRC_DIR:-$REPO_ROOT/.cache/raylib-win64-src}
RAYLIB_BUILD_DIR=${RAYLIB_BUILD_DIR:-$REPO_ROOT/.cache/raylib-win64-build}
BUILD_DIR=${BUILD_DIR:-$REPO_ROOT/build-win64}
PYTHON_BIN=${PYTHON_BIN:-python3}

mkdir -p "$REPO_ROOT/.cache"

if [ ! -d "$RAYLIB_SRC_DIR/.git" ]; then
	git clone --depth 1 https://github.com/raysan5/raylib.git "$RAYLIB_SRC_DIR"
fi

cmake -S "$RAYLIB_SRC_DIR" -B "$RAYLIB_BUILD_DIR" \
	-G "Unix Makefiles" \
	-DCMAKE_SYSTEM_NAME=Windows \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
	-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
	-DBUILD_SHARED_LIBS=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DPLATFORM=Desktop

cmake --build "$RAYLIB_BUILD_DIR" -j2

cd "$REPO_ROOT"
make generate-visualization-json CC=gcc PYTHON="$PYTHON_BIN"
C_INCLUDE_PATH="$RAYLIB_SRC_DIR/src" \
make -B \
	BUILD_DIR="$BUILD_DIR" \
	EXEEXT=.exe \
	CC=x86_64-w64-mingw32-gcc \
	PYTHON="$PYTHON_BIN" \
	BUILD_COMMIT="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)" \
	RAYLIB_LIBS="-L$RAYLIB_BUILD_DIR/raylib -lraylib -lopengl32 -lgdi32 -lwinmm" \
	all visualizer