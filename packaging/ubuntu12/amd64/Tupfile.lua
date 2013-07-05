if tup.getconfig('PLATFORM') ~= 'ubuntu12_64' then return end

DoOnce 'info.lua'
DoOnce 'app/Tupfile.lua'

ArchPackage = Define.Lua
{
	Inputs = App,
	Outputs = Item(('%s_%d_amd64.deb'):format(Info.PackageName, Info.Version)),
	Script = '../package.lua',
	Arguments = 'amd64 ' .. tostring(App)
}

