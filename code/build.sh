#!/bin/sh

# NOTE(luca): This script bootsraps cling and runs it

set -eu
ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"

mkdir -p ../build

if [ ! -x "../build/cling" ]
then
	printf '%s\n' cling.c
	cc -g -Wno-write-strings -D_GNU_SOURCE=1 -I. -o ../build/cling ./cling/cling.c
fi

CLING_NOREBUILD=1 ../build/cling "$@"
