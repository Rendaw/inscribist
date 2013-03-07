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
local TupConfig = nil

-- Start gathering info
local DebugFlag = Discover.Flag{Name = 'Debug', Description = 'Enables building a debug executable.  The debug executable will be built in ' .. Root .. 'variant-debug/.'}
Guard(function()
	if DebugFlag.Present
	then
		Debug = true
		VariantDirectory = Root .. 'variant-debug/'
	end
end)

Tup = Discover.Program{Name = {'tup', 'tup-lua'}}

Guard(function()
	Utility.MakeDirectory{Directory = VariantDirectory}
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
	local InstallData = Discover.InstallDataDirectory{Project = 'inscribist'}
	Guard(function()
		TupConfig:write('CONFIG_DATADIRECTORY=' .. InstallData.Location .. '\n')
	end)
end

local Platform = Discover.Platform()
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
	{'lua52', 'lua'}, 
	{'bz2', 'bzip2'}, 
	{'gtk-2.0', 'gtk-x11-2.0', 'gtk-win32-2.0-0'}, 
	{'gdk-2.0', 'gdk-x11-2.0', 'gdk-win32-2.0-0'}, 
	{'atk-1.0', 'atk-1.0-0'}, 
	{'gio-2.0', 'gio-2.0-0'}, 
	{'pangoft2-1.0', 'pangoft2-1.0-0'}, 
	{'pangocairo-1.0', 'pangocairo-1.0-0'}, 
	{'gdk_pixbuf-2.0', 'gdk_pixbuf-2.0-0'}, 
	{'cairo', 'cairo-2'}, 
	{'pango-1.0', 'pango-1.0-0'}, 
	{'freetype', 'freetype6'}, 
	{'fontconfig', 'fontconfig-1'}, 
	{'gobject-2.0', 'gobject-2.0-0'}, 
	{'glib-2.0', 'glib-2.0-0'}
}
if Platform.Family == 'windows'
then
	table.insert(Libraries, {'intl'})
end
for Index, Library in ipairs(Libraries)
do
	local LibraryInfo = Discover.CLibrary{Name = Library}
	Guard(function()
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
		if Platform.Family == 'windows' 
		then
			if #LibraryInfo.LibraryDirectories > 1 or #LibraryInfo.Filenames > 1
			then
				error('Was pkg-config used for library ' .. Library .. '?  Too much information, can\'t determine library files for packaging.')
			end
			table.insert(PackageLibBinaries, LibraryInfo.LibraryDirectories[1] .. '/' .. LibraryInfo.Filenames[1])
		end
		for DirectoryIndex, Directory in ipairs(LibraryInfo.LibraryDirectories) do
			LinkDirectories['-L"' .. Directory .. '"'] = true
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

-- Locate source packages for packaging on windows
local PackageSources = {}
function AskForSourcePackage(Name)
	Result = Discover.Flag{Name = Name, Description = 'The location of the source package for the ' .. Name .. ' package linked to the executable.  This is used and required on Windows only to include in the installer to comply with the license for ' .. Name .. '.', HasValue = true}
	if Result
	then
		if not Result.Present
		then
			error('On Windows, the location of ' .. Name .. ' must be specified.  Run the configure script again with Help specified for more information.')
		end
		table.insert(PackageSources, Result.Value)
	end
end
AskForSourcePackage('gettextSources')
AskForSourcePackage('bz2Sources')
AskForSourcePackage('GLibSources')
AskForSourcePackage('GTKSources')
AskForSourcePackage('GDKSources')
AskForSourcePackage('PangoSources')
AskForSourcePackage('AtkSources')

Guard(function()
	if Platform.Family == 'windows'
	then
		local PackageInclude, Error = io.open('packaging/windows/package.include.lua', 'w')
		if not PackageInclude then error(Error) end

		PackageInclude:write 'PackageSources = {\n'
		for Index, File in ipairs(PackageSources)
		do
			if not io.open(File, 'r') then error('Package source path ' .. File .. ' is invalid.') end
			PackageInclude:write('\t\'' .. File .. '\',\n')
		end
		PackageInclude:write '}\n\n'

		PackageInclude:write 'LibBinaries = {\n'
		for Index, File in ipairs(PackageLibBinaries)
		do
			PackageInclude:write('\t\'' .. File .. '\',\n')
		end
		PackageInclude:write '}\n\n'
	end
end)


