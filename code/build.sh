#!/bin/sh

# NOTE(luca): This script bootsraps cling and runs it

set -eu
ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"

mkdir -p ../build

if [ ! -x "../build/cling" ]
then
	printf '%s\n' cling.c
	cc -g -D_GNU_SOURCE=1 -I. \
		-Wno-unused-value -Wno-initializer-overrides -Wno-write-strings \
		-o ../build/cling ./cling/cling.c
fi

CLING_NOREBUILD=1 ../build/cling "$@"
