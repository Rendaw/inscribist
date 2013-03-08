#!/usr/bin/lua
dofile '../../info.include.lua'
dofile 'package.include.lua'

local UpgradeGUID = '628A99DD-AEE3-423E-B949-4781D97D98B5'
local PackageGUID = 'E2D0EB03-D081-4BFD-B2BD-87BB8F5EFDAC'
local Version = '1.' .. tostring(Info.Version) .. '.0'

function FileComponent(Indentation, ID, Filename)
	Pre = ('\t'):rep(Indentation)
	return 
		[[<Component Id="]] .. ID .. [[" Guid="*">]] .. '\n' ..
		Pre .. '\t' .. [[<File Id="]] .. ID .. [[File" Source="]] .. Filename .. [[" KeyPath="yes" />]] .. '\n' ..
		Pre .. [[</Component>]] .. '\n'
end

function PackageBinaryComponents()
	Out = {}
	for Index, File in ipairs(LibBinaries)
	do
		table.insert(Out, FileComponent(5, 'CoreComponentLib' .. Index, File))
	end
	return table.concat(Out, '\t\t\t\t\t')
end

function PackageBinaryFeatureItems()
	Out = {}
	for Index, File in ipairs(LibBinaries)
	do
		table.insert(Out, '<ComponentRef Id="CoreComponentLib' .. Index .. '" />\n')
	end
	return table.concat(Out, '\t\t\t')
end

function WebsiteShortcutComponent(Indentation, ID, Label, URL)
	Pre = ('\t'):rep(Indentation)
	return 
		[[<Component Id="]] .. ID .. [[" Guid="*">]] .. '\n' ..
		Pre .. '\t' .. [[<util:InternetShortcut Id="]] .. ID .. [[File" Name="]] .. Label .. [[" Target="]] .. URL .. [[" />]] .. '\n' ..
		Pre .. '\t' .. [[<RegistryValue Root="HKCU" Key="Software\]] .. Info.Company .. [[\]] .. Info.PackageName .. [[" Name="]] .. ID .. [[" Type="integer" Value="1" KeyPath="yes"/>]] .. '\n' ..
		Pre .. [[</Component>]] .. '\n'
end


io.open('./install_' .. Info.PackageName .. '.wxs', 'w+'):write([[
<?xml version="1.0"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi" xmlns:util="http://schemas.microsoft.com/wix/UtilExtension">
	<Product Id="*" UpgradeCode="]] .. UpgradeGUID .. [[" Name="]] .. Info.PackageName .. [[" Version="]] .. Version .. [[" Manufacturer="]] .. Info.Company .. [[" Language="1033">
		<Package InstallerVersion="200" Compressed="yes" Comments="Windows Installer Package" />
		<Media Id="1" Cabinet="product.cab" EmbedCab="yes" />
		<Icon Id="Icon32" SourceFile="..\..\data\icon32.ico" />
		<Property Id="ARPPRODUCTICON" Value="Icon32" />
		<Property Id="ARPHELPLINK" Value="]] .. Info.Website .. [[" />
		<Property Id="ARPURLINFOABOUT" Value="]] .. Info.CompanyWebsite .. [[" />

		<!-- AdvancedUI stuff - also requires APPLICATIONFOLDER as Id below -->
		<UIRef Id="WixUI_Advanced" />
		<WixVariable Id="WixUILicenseRtf" Value="licenses.rtf" />
		<Property Id="ApplicationFolderName" Value="]] .. Info.PackageName .. [[" />
		<Property Id="WixAppFolder" Value="WixPerMachineFolder" />

		<Directory Id="TARGETDIR" Name="SourceDir">
			 <Directory Id="ProgramFilesFolder">
				<Directory Id="APPLICATIONFOLDER" Name="]] .. Info.PackageName .. [[">
					<Component Id="CoreComponent" Guid="*">
						<File Id="CoreComponentFile" Source="..\..\app\build\inscribist.exe" KeyPath="yes" Checksum="yes" />
					</Component>
					]] .. PackageBinaryComponents() .. [[
					]] .. FileComponent(5, 'CoreLicense', '..\\..\\license.txt') .. [[
					]] .. FileComponent(5, 'LuaLicense', 'lualicense.txt') .. [[
					]] .. WebsiteShortcutComponent(5, 'CoreWebsiteShortcut', Info.PackageName .. ' Website', Info.Website) .. [[
					]] .. WebsiteShortcutComponent(5, 'CoreForumShortcut', Info.PackageName .. ' Forum', Info.Forum) .. [[
				</Directory>
			 </Directory>

			 <Directory Id="ProgramMenuFolder">
			 	<Directory Id="ProgramMenuSubfolder" Name="]] .. Info.PackageName .. [[">
					<Component Id="ApplicationShortcuts" Guid="*">
						<Shortcut Id="ApplicationShortcutFile" Name="]] .. Info.PackageName .. [[" Target="[APPLICATIONFOLDER]inscribist.exe" WorkingDirectory="APPLICATIONFOLDER" Icon="Icon32" />
						<RegistryValue Root="HKCU" Key="Software\]] .. Info.Company .. [[\]] .. Info.PackageName .. [[" Name="ApplicationShortcutFile" Type="integer" Value="1" KeyPath="yes"/>
						<RemoveFolder Id="ProgramMenuSubfolder" On="uninstall"/>
					</Component>
					]] .. WebsiteShortcutComponent(5, 'StartMenuWebsiteShortcut', Info.PackageName .. ' Website', Info.Website) .. [[
					]] .. WebsiteShortcutComponent(5, 'StartMenuForumShortcut', Info.PackageName .. ' Forum', Info.Forum) .. [[
				</Directory>
			 </Directory>
		</Directory>

		<Feature Id="Core" Level="1" Absent="disallow" Title="Everything">
			<ComponentRef Id="CoreComponent" />
			]] .. PackageBinaryFeatureItems() .. [[
			<ComponentRef Id="CoreLicense" />
			<ComponentRef Id="LuaLicense" />
			<ComponentRef Id="CoreWebsiteShortcut" />
			<ComponentRef Id="CoreForumShortcut" />
			<ComponentRef Id="ApplicationShortcuts" />
			<ComponentRef Id="StartMenuWebsiteShortcut" />
			<ComponentRef Id="StartMenuForumShortcut" />
		</Feature>
	</Product>
</Wix>
]]):close()

