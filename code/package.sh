#!/bin/sh

set -eu
ScriptDirectory="$(dirname "$(readlink -f "$0")")"
cd "$ScriptDirectory"/../build

release=0
for arg in "$@"; do eval "$arg=1"; done

suffix=""
mode=""
if [ "$release" = "1" ]
then 
 mode="-Release"
 suffix="release"
else
 suffix="debug"
fi

sh ../code/build.sh -Muze $mode -Slow

dir="muze_$suffix"
#--- Create clean directory
rm -rf "$dir"
mkdir "$dir"
cd "$dir"

cp -v ../../build/muze .
cp -v ../../build/muze_app.so .
cp -v ../../data/font_regular.ttf .
cp -v ../../data/icons.ttf .
cp -v ../../data/muze/sounds/sounds.sf2 .
cp -v ../../data/muze/song.muze_piece .

rm -f *.zip
zip "$dir.zip" *
