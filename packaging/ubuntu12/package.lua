#!/usr/bin/lua
local arg = arg
_G.arg = nil
dofile '../../../info.lua'

os.execute('mkdir -p ' .. Info.PackageName .. '/usr/bin')
for FileIndex = 2, #arg
do
	os.execute('cp ' .. arg[FileIndex] .. ' ' .. Info.PackageName .. '/usr/bin')
end
os.execute('mkdir -p ' .. Info.PackageName .. '/usr/share/' .. Info.PackageName)
os.execute('cp ../../../data/* ' .. Info.PackageName .. '/usr/share/' .. Info.PackageName)
os.execute('mkdir -p ' .. Info.PackageName .. '/usr/share/doc/' .. Info.PackageName)
os.execute('cp ../../../license.txt ' .. Info.PackageName .. '/usr/share/doc/' .. Info.PackageName)

local InstalledSize = io.popen('du -s -BK ' .. Info.PackageName):read():gsub('[^%d].*$', '')
print('Installed size is ' .. InstalledSize)

os.execute('mkdir -p ' .. Info.PackageName .. '/DEBIAN')
io.open(Info.PackageName .. '/DEBIAN/control', 'w+'):write([[
Package: ]] .. Info.PackageName .. [[

Version: ]] .. Info.Version .. [[

Section: Graphics
Priority: Optional
Architecture: ]] .. arg[1] .. [[

Depends: libstdc++6 (>= 4.7.0-7ubuntu3), lua5.2 (>= 5.2.0-2)
Maintainer: ]] .. Info.Author .. ' <' .. Info.EMail .. [[>
Description: ]] .. Info.ExtendedDescription .. [[

Installed-Size: ]] .. InstalledSize .. [[

Homepage: ]] .. Info.Website .. [[

]]):close()

os.execute('fakeroot dpkg --build ' .. Info.PackageName .. ' .')
os.execute('rm -r ' .. Info.PackageName)

