#!/usr/bin/lua
local arg = arg
_G.arg = nil
dofile '../../info.lua'

local Base = function(Filename)
	return (Filename:gsub('.*/', ''))
end

local Here = io.popen('pwd'):read() .. '/'

os.execute('mkdir temp')
io.open('temp/PKGBUILD', 'w+'):write([[
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

package() {
	mkdir -p $pkgdir/usr/bin/
]] .. (function()
	local Out = {}
	for FileIndex = 1, #arg
	do
		Out[#Out + 1] = '\tcp ' .. Here .. arg[FileIndex] .. ' $pkgdir/usr/bin/\n'
	end
	return table.concat(Out)
end)() .. [[
	mkdir -p $pkgdir/usr/share
	cp ]] .. Here .. [[../../data/* $pkgdir/usr/share
	mkdir -p $pkgdir/usr/share/licenses/$pkgname
	cp ]] .. Here .. [[../../license.txt $pkgdir/usr/share/licenses/$pkgname/
}
]]):close()

os.execute('mkdir temp/src')
os.execute('cd temp && makepkg --repackage --noextract --nocheck --force')
os.execute('cp temp/' .. Info.PackageName .. '-' .. tostring(Info.Version) .. '-1-x86_64.pkg.tar.xz .')
os.execute('cp temp/PKGBUILD .')
os.execute('rm -r temp')

