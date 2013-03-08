local Root = Discover.ControllerLocation() .. '/'

dofile(Root .. 'info.include.lua')

-- Setup
Discover.Version({Version = 1})

local Help = Discover.HelpMode()

local Guard = function(Action) -- Helps prevent filesystem changes in help mode
	if not Help then Action() end
end

local Debug = false
local VariantDirectory = Root .. 'variant-release/'
TupConfig = nil

-- Start gathering info
local DebugFlag = Discover.Flag{Name = 'Debug', Description = 'Enables building a debug executable.  The debug executable will be built in ' .. Root .. 'variant-debug/.'}
local Platform = Discover.Platform()

if DebugFlag.Present
then
	Debug = true
	VariantDirectory = Root .. 'variant-debug/'
end
if Platform.Family == 'windows'
then
	VariantDirectory = Root
end

Tup = Discover.Program{Name = {'tup', 'tup-lua'}}

Guard(function()
	if Platform.Family ~= 'windows' 
	then
		Utility.MakeDirectory{Directory = VariantDirectory}
	end
	Utility.Call{Command = Tup.Location .. ' init', WorkingDirectory = Root}
	local Error
	TupConfig, Error = io.open(VariantDirectory .. 'tup.config', 'w')
	if not TupConfig then error(Error) end
	if Debug then TupConfig:write 'CONFIG_DEBUG=true\n' end
	TupConfig:write('CONFIG_ROOT=' .. Root .. '\n')
	TupConfig:write('CONFIG_VERSION=' .. Info.Version .. '\n')
	TupConfig:write('CONFIG_CFLAGS=' .. (os.getenv 'CFLAGS' or '') .. '\n')
end)

if Debug
then
	Guard(function()
		TupConfig:write('CONFIG_DATADIRECTORY=' .. Root .. 'data/\n')
	end)
else
	if Platform.Family == 'windows'
	then
		Guard(function()
			TupConfig:write('CONFIG_DATADIRECTORY=.\n')
		end)
	else
		local InstallData = Discover.InstallDataDirectory{Project = 'inscribist'}
		Guard(function()
			TupConfig:write('CONFIG_DATADIRECTORY=' .. InstallData.Location .. '\n')
		end)
	end
end

Guard(function()
	TupConfig:write('CONFIG_PLATFORM=')
	if Platform.Family == 'windows'
	then
		TupConfig:write 'windows\n'
	else
		TupConfig:write 'linux\n'
	end
end)

local Compiler = Discover.CXXCompiler{CXX11 = true}
Guard(function()
	TupConfig:write('CONFIG_COMPILERNAME=' .. Compiler.Name .. '\n')
	TupConfig:write('CONFIG_COMPILER=' .. Compiler.Path .. '\n')
end)

-- Locate libraries
local LinkDirectories = {}
local IncludeDirectories = {}
local LinkFlags = {}
local PackageLibBinaries = {}

local Libraries = {
	{ Name = {'lua52', 'lua'}}, 
	{ Name = {'bz2', 'bzip2'}}, 
	{ Name = {'gtk-2.0', 'gtk-x11-2.0', 'gtk-win32-2.0-0'}}, 
	{ Name = {'gdk-2.0', 'gdk-x11-2.0', 'gdk-win32-2.0-0'}}, 
	{ Name = {'atk-1.0', 'atk-1.0-0'}}, 
	{ Name = {'gio-2.0', 'gio-2.0-0'}}, 
	{ Name = {'pangoft2-1.0', 'pangoft2-1.0-0'}}, 
	{ Name = {'pangocairo-1.0', 'pangocairo-1.0-0'}}, 
	{ Name = {'gdk_pixbuf-2.0', 'gdk_pixbuf-2.0-0'}}, 
	{ Name = {'cairo', 'cairo-2'}}, 
	{ Name = {'pango-1.0', 'pango-1.0-0'}}, 
	{ Name = {'freetype', 'freetype6'}}, 
	{ Name = {'fontconfig', 'fontconfig-1'}}, 
	{ Name = {'gobject-2.0', 'gobject-2.0-0'}}, 
	{ Name = {'glib-2.0', 'glib-2.0-0'}},
	{ Name = {'intl'}, Windows = true},
	{ Name = {'expat-1'}, Windows = true, NoLink = true},
	{ Name = {'png14-14'}, Windows = true, NoLink = true},
	{ Name = {'zlib1'}, Windows = true, NoLink = true},
	{ Name = {'gmodule-2.0-0'}, Windows = true, NoLink = true},
	{ Name = {'pangowin32-1.0-0'}, Windows = true, NoLink = true},
	{ Name = {'gthread-2.0-0'}, Windows = true, NoLink = true},
	{ Name = {'gcc_s_sjlj-1'}, Windows = true, NoLink = true},
	{ Name = {'stdc++-6'}, Windows = true, NoLink = true}
}

function AddPackageLibBinary(LibraryInfo)
end

for Index, Library in ipairs(Libraries)
do
	local LibraryInfo = Discover.CLibrary{Name = Library.Name}
	Guard(function()
		if (not Library.Windows or Platform.Family == 'windows') and not Library.NoLink
		then
			for DirectoryIndex, Directory in ipairs(LibraryInfo.IncludeDirectories) do
				IncludeDirectories['-I"' .. Directory .. '"'] = true
			end
			for FilenameIndex, Filename in ipairs(LibraryInfo.Filenames) do
				local ShortName = Filename:gsub('^lib', '')
				ShortName = ShortName:gsub('%.so$', '')
				ShortName = ShortName:gsub('%.so%..*', '')
				ShortName = ShortName:gsub('%.dll$', '')
				LinkFlags['-l' .. ShortName] = true
			end
			for DirectoryIndex, Directory in ipairs(LibraryInfo.LibraryDirectories) do
				LinkDirectories['-L"' .. Directory .. '"'] = true
			end
		end
		if Platform.Family == 'windows'
		then
			if #LibraryInfo.LibraryDirectories > 1 or #LibraryInfo.Filenames > 1
			then
				error('Was pkg-config used for library ' .. Library .. '?  Too much information, can\'t determine library files for packaging.')
			end
			table.insert(PackageLibBinaries, LibraryInfo.LibraryDirectories[1] .. '/' .. LibraryInfo.Filenames[1])
		end
	end)
end
Guard(function()
	TupConfig:write 'CONFIG_LIBDIRECTORIES='
	for Flag, Discard in pairs(LinkDirectories) do TupConfig:write(Flag .. ' ') end
	TupConfig:write '\n'

	TupConfig:write 'CONFIG_INCLUDEDIRECTORIES='
	for Flag, Discard in pairs(IncludeDirectories) do TupConfig:write(Flag .. ' ') end
	TupConfig:write '\n'

	TupConfig:write 'CONFIG_LINKFLAGS='
	for Flag, Discard in pairs(LinkFlags) do TupConfig:write(Flag .. ' ') end
	TupConfig:write '\n'
end)

Guard(function()
	if Platform.Family == 'windows'
	then
		local PackageInclude, Error = io.open('packaging/windows/package.include.lua', 'w')
		if not PackageInclude then error(Error) end

		PackageInclude:write 'LibBinaries = {\n'
		for Index, File in ipairs(PackageLibBinaries)
		do
			PackageInclude:write('\t\'' .. File .. '\',\n')
		end
		PackageInclude:write '}\n\n'
	end
end)


