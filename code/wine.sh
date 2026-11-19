#!/bin/sh

set -eu
ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"

cat <<EOF | wine cmd.exe 2>/dev/null
call build.bat $@
EOF
