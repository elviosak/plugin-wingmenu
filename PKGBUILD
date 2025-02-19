pkgname=lxqt-wingmenu-git
pkgver=r1
pkgrel=1
pkgdesc="Wing Menu is an alternative menu plugin for lxqt-panel"
arch=("x86_64")
url="https://github.com/slidinghotdog/plugin-wingmenu"
license=("unknown")
depends=("lxqt-panel")
#optdepends=()
makedepends=("git" "liblxqt" "lxqt-build-tools" "lxqt-panel")
provides=("wingmenu")
source=("git+https://github.com/slidinghotdog/plugin-wingmenu")
sha256sums=("SKIP")

pkgver() {
  cd "$srcdir/plugin-wingmenu"
  ( set -o pipefail
    git describe --long 2>/dev/null | sed 's/\([^-]*-g\)/r\1/;s/-/./g' ||
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
  )
}

build() {
	cmake -B build -S "$srcdir/plugin-wingmenu" \
		-DCMAKE_INSTALL_PREFIX=/usr
	make -C build
}

package() {
	cd build
	make DESTDIR="$pkgdir" install
}
