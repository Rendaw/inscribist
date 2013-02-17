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

Guard(function()
	Utility.MakeDirectory{Directory = VariantDirectory}
	Utility.Call{Command = 'tup-lua init', WorkingDirectory = Root}
	local Error
	TupConfig, Error = io.open(VariantDirectory .. 'tup.config', 'w')
	if not TupConfig then error(Error) end
	if Debug then TupConfig:write 'CONFIG_DEBUG=true\n' end
	TupConfig:write('CONFIG_ROOT=' .. Root .. '\n')
	TupConfig:write('CONFIG_VERSION=' .. Info.Version .. '\n')
	TupConfig:write('CONFIG_CFLAGS=' .. os.getenv 'CFLAGS' .. '\n')
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

local LinkDirectories = {}
local IncludeDirectories = {}
local LinkFlags = {}

local Libraries = {{'lua52', 'lua'}, 'bz2', {'gtk-2.0', 'gtk-x11-2.0'}, {'gdk-2.0', 'gdk-x11-2.0'}, 'atk-1.0', 'gio-2.0', 'pangoft2-1.0', 'pangocairo-1.0', 'gdk_pixbuf-2.0', 'cairo', 'pango-1.0', 'freetype', 'fontconfig', 'gobject-2.0', 'glib-2.0'}
for Index, Library in ipairs(Libraries)
do
	local LibraryInfo = Discover.CLibrary{Name = Library}
	Guard(function()
		for DirectoryIndex, Directory in ipairs(LibraryInfo.IncludeDirectories) do
			IncludeDirectories['-I' .. Directory] = true
		end
		for FilenameIndex, Filename in ipairs(LibraryInfo.Filenames) do
			local ShortName = Filename:gsub('^lib', '')
			ShortName = ShortName:gsub('%.so$', '')
			ShortName = ShortName:gsub('%.so%..*', '')
			LinkFlags['-l' .. ShortName] = true
		end
		for DirectoryIndex, Directory in ipairs(LibraryInfo.LibraryDirectories) do
			LinkDirectories['-L' .. Directory] = true
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
