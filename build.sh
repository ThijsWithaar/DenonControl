#!/bin/sh
cmake -GNinja -S. -B$1 -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build $1 --target package
mkdir -p deploy
cp $1/*.zip $1/*.deb $1/*.rpm deploy/ | true
