#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/apex3d-cp2.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

CXX="${CXX:-c++}"
FLAGS=(-std=c++20 -Wall -Wextra -Wpedantic)

build_and_run() {
    local name="$1"
    local source="$2"
    shift 2
    echo "[CP2] Building $name"
    "$CXX" "${FLAGS[@]}" "$ROOT/$source" -o "$BUILD_DIR/$name"
    echo "[CP2] Running $name"
    "$BUILD_DIR/$name" "$@"
}

build_and_run transform-tests checkpoint2/transformations/tests.cpp
build_and_run transform-demo checkpoint2/transformations/main.cpp \
    "$BUILD_DIR/transformations.ppm"

build_and_run timeline-tests checkpoint2/animation/timeline/tests.cpp
build_and_run timeline-demo checkpoint2/animation/timeline/main.cpp \
    "$BUILD_DIR/timeline"
build_and_run timeline-sheet checkpoint2/animation/timeline/contact_sheet.cpp \
    "$BUILD_DIR/timeline" "$BUILD_DIR/timeline"

build_and_run wheel-tests checkpoint2/animation/wheels/tests.cpp
build_and_run wheel-demo checkpoint2/animation/wheels/main.cpp \
    "$BUILD_DIR/wheels"

build_and_run car-tests checkpoint2/animation/car/tests.cpp
build_and_run car-demo checkpoint2/animation/car/main.cpp \
    "$BUILD_DIR/car"

build_and_run spline-tests checkpoint2/animation/splines/tests.cpp
build_and_run spline-demo checkpoint2/animation/splines/main.cpp \
    "$BUILD_DIR/splines"

build_and_run bezier-tests checkpoint2/animation/bezier/tests.cpp
build_and_run bezier-demo checkpoint2/animation/bezier/main.cpp \
    "$BUILD_DIR/bezier"

echo "[CP2] All strict builds, tests and demonstrations passed."
