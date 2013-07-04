if not arg[1] or not arg[2]
then
	error 'Usage: lua txttortf.lua OUT IN...'
end

	
local Outfile = io.open(arg[1], 'w+')
Outfile:write([[{\rtf1\ansi{\fonttbl\f0\fmodern Pica;}\f0\pard]] .. '\n')

for FileIndex = 2, #arg
do
	local Infile = io.open(arg[FileIndex], 'r')
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

