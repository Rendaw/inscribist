if not arg[1] or not arg[2]
then
	error 'Usage: lua txttortf.lua OUT (LABEL IN)...'
end

	
local Outfile = io.open(arg[1], 'w+')
Outfile:write([[{\rtf1\ansi{\fonttbl\f0\fmodern Pica;}\f0\pard]] .. '\n')

for FileIndex = 2, #arg, 2
do
	Outfile:write(arg[FileIndex] .. '\\par \n\\par \n')
	local Infile = io.open(arg[FileIndex + 1], 'r')
	while true
	do
		Line = Infile:read()
		if not Line then break end
		Outfile:write(Line .. '\\par \n')
	end
	Infile:close()
	Outfile:write('\\par \n\\par \n\\par \n')
end
Outfile:close()

