#!/bin/sh

ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"

set -eux

~/src/mcs51/asm51/asm51 -a -n -o ../../../build/main.hex main.asm
../../../build/lsd ../../../build/main.hex

