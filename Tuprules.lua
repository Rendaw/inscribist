-- Template top-level Tuprules.lua
tup.include 'tupsupport/support.lua'

-- Overrides for defaults
local BuildFlags = tup.getconfig('BUILDFLAGS') .. ' ' .. tup.getconfig('GTKBUILDFLAGS') .. ' ' .. tup.getconfig('LUABUILDFLAGS')
local LinkFlags = tup.getconfig('LINKFLAGS') .. ' ' .. tup.getconfig('GTKLINKFLAGS') .. ' ' .. tup.getconfig('LUALINKFLAGS')

local OldObject = Define.Object
Define.Object = function(Arguments)
	return OldObject(Mask({BuildFlags = BuildFlags .. ' ' .. (Arguments.BuildFlags or '')}, Arguments))
end

local OldExecutable = Define.Executable
Define.Executable = function(Arguments)
	return OldExecutable(Mask({LinkFlags = LinkFlags .. ' ' .. (Arguments.LinkFlags or '')}, Arguments))
end

