pkgname=denon
pkgver=0.2
pkgrel=1
pkgdesc="C++ library and Qt UI for controlling Denon A/V receivers"
arch=('i686' 'x86_64')
license=('GPLv2')
depends=('boost-libs')
makedepends=('cmake')
srcdir=build.arch

build() {
	cmake -GNinja -S.. -Bbuild -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX:PATH=$pkgdir
	cmake --build build
}

check() {
  : cmake --build build --target tests
}

package() {
	cmake --build build --target install
}
