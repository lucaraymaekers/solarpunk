#!/bin/sh

ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"

Source="$(readlink -f "main.asm")"

cd ../../../build

set -eux

# Assemble
asm51 -a -n -o main.hex "$Source"

# Download
./lsd -verbose -run main.hex
