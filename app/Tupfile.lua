DoOnce('app/ren-general/Tupfile.lua')
DoOnce('app/ren-script/Tupfile.lua')
DoOnce('app/ren-translation/Tupfile.lua')
DoOnce('app/ren-gtk/Tupfile.lua')

local SharedSources = Item{'image.cxx', 'settings.cxx'}
ImageObject = Define.Object
{
	Source = Item 'image.cxx',
	BuildFlags = tup.getconfig('GTKBUILDFLAGS')
}
SettingsObject = Define.Object
{
	Source = Item 'settings.cxx',
	BuildFlags = tup.getconfig('GTKBUILDFLAGS')
}

App = Define.Executable
{
	Name = 'inscribist',
	Sources = Item '*.cxx':Exclude 'test.cxx':Exclude(SharedSources),
	Objects = Item()
		:Include(ImageObject):Include(SettingsObject)
		:Include(GeneralObjects):Include(ScriptObjects):Include(TranslationObjects):Include(GTKObjects),
	BuildFlags = tup.getconfig('GTKBUILDFLAGS'),
	LinkFlags = tup.getconfig('GTKLINKFLAGS')
}

Test = Define.Executable
{
	Name = 'test',
	Sources = Item 'test.cxx',
	Objects = {ImageObject, SettingsObject}
}

