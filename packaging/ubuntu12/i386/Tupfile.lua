if tup.getconfig('PLATFORM') ~= 'ubuntu12' then return end

DoOnce 'info.lua'
DoOnce 'app/Tupfile.lua'

ArchPackage = Define.Lua
{
	Inputs = App,
	Outputs = Item(('%s_%d_i386.deb'):format(Info.PackageName, Info.Version)),
	Script = '../package.lua',
	Arguments = 'i386 ' .. tostring(App)
}

