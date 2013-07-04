#!/usr/bin/lua
dofile '../../info.include.lua'

local Variant = '../../variant-release'
if arg[1] and arg[1] == 'debug' then Variant = '../../variant-debug' end

if not os.execute('test -d ' .. Variant)
then
	error('You must have a build in variant directory ' .. Variant .. ' for this to work.')
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

os.execute('mkdir -p src')
os.execute('cp ' .. Variant .. '/build/inscribist src')
os.execute('cp ../../license.txt src')
os.execute('makepkg --repackage --force')
os.execute('rm PKGBUILD')

