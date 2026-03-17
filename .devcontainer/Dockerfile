FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG RAYLIB_REF=master

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        binutils-mingw-w64-x86-64 \
        build-essential \
        ca-certificates \
        clang \
        clang-tidy \
        cmake \
        cppcheck \
        g++-mingw-w64-x86-64 \
        gcc \
        gcc-mingw-w64-x86-64 \
        gdb \
        git \
        jq \
        lcov \
        libc6-dbg \
        libclang-rt-18-dev \
        libasound2-dev \
        libgl1-mesa-dev \
        libx11-dev \
        libxcursor-dev \
        libxi-dev \
        libxinerama-dev \
        libxrandr-dev \
        llvm \
        make \
        pkg-config \
        python3 \
        python3-venv \
        valgrind \
        wine64 \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch ${RAYLIB_REF} https://github.com/raysan5/raylib.git /tmp/raylib \
    && cmake -S /tmp/raylib -B /tmp/raylib-build \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_GAMES=OFF \
    && cmake --build /tmp/raylib-build -j"$(nproc)" \
    && cmake --install /tmp/raylib-build \
    && ldconfig \
    && rm -rf /tmp/raylib /tmp/raylib-build