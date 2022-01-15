#!/usr/bin/env bash

set -e                     # fail if any command has a non-zero exit status
set -u                     # fail if any undefined variable is referenced
set -o pipefail            # propagate failure exit status through a pipeline
shopt -s globstar nullglob # enable recursive and null globbing

exe_name="co2minimon"
build_dir="./out"

cc=clang
source_files=("main.c")
cflags=("-std=c11" "-D_XOPEN_SOURCE=700" "-Wall" "-Wextra" "-Wpedantic" "-Wsign-compare" "-Wswitch-enum")
debug_flags=("-g" "-O0")
release_flags=("-O2" "-Os")

build_release() {
    release_dir="${build_dir}/release"
    release_path="${release_dir}/${exe_name}"
    mkdir -p "$release_dir"
    (
        set -x;
        "$cc" "${cflags[@]}" "${release_flags[@]}" "${source_files[@]}" -o "$release_path"
    )
}

build_debug() {
    debug_dir="${build_dir}/debug"
    debug_path="${debug_dir}/${exe_name}"
    mkdir -p "$debug_dir"
    (
        set -x;
        "$cc" "${cflags[@]}" "${debug_flags[@]}" "${source_files[@]}" -o "$debug_path"
    )
}

usage() {
    echo "build.sh - Build ${exe_name}"
    echo " "
    echo "build.sh [options]"
    echo " "
    echo "options:"
    echo "-h, --help  show help"
    echo "-d, --debug build only debug executable"
}

debug_build=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -d|--debug)
            debug_build=true
            shift
            ;;
        *)
            break
            ;;
    esac
done

if [ "$debug_build" = true ]; then
    build_debug
else
    build_release
fi

exit 0
