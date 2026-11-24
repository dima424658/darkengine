#!/usr/bin/env sh
set -eu

mkdir -p .ai
cmake --build build-vita 2> .ai/compiler_errors.log
