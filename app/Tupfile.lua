DoOnce('app/ren-general/Tupfile.lua')
DoOnce('app/ren-script/Tupfile.lua')
DoOnce('app/ren-translation/Tupfile.lua')
DoOnce('app/ren-gtk/Tupfile.lua')

local SharedSources = Item():Include 'image.cxx':Include 'settings.cxx'
ImageObject = Define.Object
{
	Source = Item 'image.cxx',
}
SettingsObject = Define.Object
{
	Source = Item 'settings.cxx',
}
InfoHeader = Define.Lua
{
	Outputs = Item 'info.h',
	Script = 'info2h.lua'
}

local LinkFlags
LinkFlags = '-lbz2'
App = Define.Executable
{
	Name = 'inscribist',
	Sources = Item '*.cxx':Exclude 'test.cxx':Exclude(SharedSources),
	Objects = Item()
		:Include(ImageObject):Include(SettingsObject)
		:Include(GeneralObjects):Include(ScriptObjects):Include(TranslationObjects):Include(GTKObjects),
	BuildExtras = InfoHeader,
	LinkFlags = LinkFlags
}

Test = Define.Executable
{
	Name = 'test',
	Sources = Item 'test.cxx',
	Objects = Item():Include(ImageObject):Include(SettingsObject)
		:Include(GeneralObjects):Include(ScriptObjects):Include(TranslationObjects),
	LinkFlags = LinkFlags
}

