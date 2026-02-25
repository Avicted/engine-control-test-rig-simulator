#!/bin/sh

# Wrapper so that lcov --gcov-tool can invoke "llvm-cov gcov" as a single executable.
exec llvm-cov gcov "$@"
