if tup.getconfig('PLATFORM') == 'windows'
then
	DoOnce 'info.lua'
	tup.definerule{
		inputs = {'../../info.lua', 'config.lua', 'generatewxs.lua'}, 
		outputs = {'install_' .. Info.PackageName .. '.wxs'},
		command = 'lua ./generatewxs.lua'
	}
	tup.definerule{
		inputs = {'install_' .. Info.PackageName .. '.wxs'},
		outputs = {'install_' .. Info.PackageName .. '.wixobj'},
		command = 'candle -ext WiXUtilExtension ./install_' .. Info.PackageName .. '.wxs'
	}
	Licenses = {'../../license.txt', 'lualicense.include.rtf', 'lualicense.txt', 'lgpllicense.include.rtf', 'lgpllicense.txt'}
	tup.definerule{
		inputs = Licenses,
		outputs = {'licenses.rtf'},
		command = 'lua ./txttortf.lua licenses.rtf ' .. table.concat(Licenses, ' ')
	}
	tup.definerule{
		inputs = {'install_' .. Info.PackageName .. '.wixobj'},
		outputs = {'install_' .. Info.PackageName .. '.msi'},
		command = 'light -ext WixUIExtension -ext WiXUtilExtension install_' .. Info.PackageName .. '.wixobj'
	}
end

