call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
SET VCPKG_ROOT=C:\build\vcpkg

%VCPKG_ROOT%\vcpkg.exe install boost-beast boost-system boost-program-options boost-property-tree boost-serialization
%VCPKG_ROOT%\vcpkg.exe install qt5-base

if NOT exist build (
	cmake -G "Visual Studio 16 2019" -A x64 -T v142 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -Bbuild -S.
)

cmake --build build --config MinSizeRel --target package

pause
