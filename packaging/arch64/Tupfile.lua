DoOnce 'info.lua'
DoOnce 'app/Tupfile.lua'

ArchPackage = Define.Lua
{
	Inputs = App,
	Outputs = Item(('%s-%d-1-x86_64.pkg.tar.xz'):format(Info.PackageName, Info.Version)):Include('PKGBUILD'),
	Script = 'package.lua',
	Arguments = tostring(App)
}

