#!/usr/bin/lua
dofile '../../info.include.lua'

if not os.execute 'test -d ../../variant-release'
then
	error 'You must have a release build in variant directory variant-release/ for this to work.'
end

io.open('PKGBUILD', 'w+'):write([[
pkgname=]] .. Info.PackageName .. [[

pkgver=]] .. Info.Version .. [[

pkgrel=1
epoch=
pkgdesc="]] .. Info.ShortDescription .. [["
arch=('x86_64')
url="]] .. Info.Website .. [["
license=('BSD')
groups=()
depends=('lua>=5.1', 'gtk2>=2.24.10-3')
makedepends=()
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=($pkgname-$pkgver.tar.gz)
noextract=()
md5sums=('')

build() {
	echo "Build the program first then run ./package.lua, not makepkg directly." 1>&2
	return 1
}

check() {
	echo Nop 2>&- 1>&-
}

package() {
	mkdir -p $pkgdir/usr/bin/
	mv $srcdir/inscribist $pkgdir/usr/bin/
	mkdir -p $pkgdir/usr/share
	mv $srcdir/data/* $pkgdir/usr/share
	mkdir -p $pkgdir/usr/share/licenses/$pkgname
	cp $srcdir/license.txt $pkgdir/usr/share/licenses/$pkgname/
}
]]):close()

os.execute 'mkdir -p src'
os.execute 'cp ../../variant-release/build/inscribist src'
os.execute 'cp -r ../../data src'
os.execute 'cp ../../license.txt src'
os.execute 'makepkg --repackage --force'
os.execute 'rm -r src'

