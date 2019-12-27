#!/bin/sh
cmake -GNinja -S. -Bbuild -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build --target package

