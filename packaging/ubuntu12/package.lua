#!/usr/bin/lua
dofile '../../info.include.lua'

if not os.execute 'test -d ../../variant-release'
then
	error 'You must have a release build in variant directory variant-release/ for this to work.'
end

os.execute 'mkdir -p inscribist/DEBIAN'
io.open('inscribist/DEBIAN/control', 'w+'):write([[
Package: ]] .. Info.PackageName .. [[

Version: ]] .. Info.Version .. [[

Section: Development
Priority: Optional
Architecture: all
Depends: libstdc++6 (>= 4.7.0-7ubuntu3)
Maintainer: ]] .. Info.Author .. ' <' .. Info.EMail .. [[>
Description: ]] .. Info.ExtendedDescription .. [[

Homepage: ]] .. Info.Website .. [[

]]):close()

os.execute 'mkdir -p inscribist/usr/bin'
os.execute 'cp ../../variant-release/app/build/inscribist inscribist/usr/bin'
os.execute 'mkdir -p inscribist/usr/share/doc/inscribist'
os.execute 'cp ../../license.txt inscribist/usr/share/doc/inscribist'
os.execute 'dpkg --build inscribist .'
os.execute 'rm -r inscribist/usr'

