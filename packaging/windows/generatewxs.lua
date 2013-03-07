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

function PackageSourceComponents()
	Out = {}
	for Index, File in ipairs(PackageSources)
	do
		table.insert(Out, FileComponent(5, 'CoreComponentSource' .. Index, File))
	end
	return table.concat(Out, '\t\t\t\t\t')
end

function PackageSourceFeatureItems()
	Out = {}
	for Index, File in ipairs(PackageSources)
	do
		table.insert(Out, '<ComponentRef Id="CoreComponentSource' .. Index .. '" />\n')
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
		<!--<UIRef Id="WixUI_Advanced" />
		<WixVariable Id="WixUILicenseRtf" Value="licenses.rtf" />-->

		<Directory Id="TARGETDIR" Name="SourceDir">
			 <Directory Id="ProgramFilesFolder">
				<Directory Id="INSTALLDIR" Name="]] .. Info.PackageName .. [[">
					<Component Id="CoreComponent" Guid="*">
						<File Id="CoreComponentFile" Source="..\..\app\build\inscribist.exe" KeyPath="yes" Checksum="yes" />
					</Component>
					]] .. PackageBinaryComponents() .. [[
					<Component Id="CoreLicense" Guid="*">
						<File Id="CoreLicenseFile" Source="..\..\license.txt" KeyPath="yes" />
					</Component>
					]] .. WebsiteShortcutComponent(5, 'CoreWebsiteShortcut', Info.PackageName .. ' Website', Info.Website) .. [[
					]] .. WebsiteShortcutComponent(5, 'CoreForumShortcut', Info.PackageName .. ' Forum', Info.Forum) .. [[
					]] .. PackageSourceComponents() .. [[
				</Directory>
			 </Directory>

			 <Directory Id="ProgramMenuFolder">
				<Component Id="ApplicationShortcuts" Guid="*">
					<Shortcut Id="ApplicationShortcutFile" Name="]] .. Info.PackageName .. [[" Target="[INSTALLDIR]inscribist.exe" WorkingDirectory="INSTALLDIR" Icon="Icon32" />
					<RegistryValue Root="HKCU" Key="Software\]] .. Info.Company .. [[\]] .. Info.PackageName .. [[" Name="ApplicationShortcutFile" Type="integer" Value="1" KeyPath="yes"/>
				</Component>
				]] .. WebsiteShortcutComponent(4, 'StartMenuWebsiteShortcut', Info.PackageName .. ' Website', Info.Website) .. [[
				]] .. WebsiteShortcutComponent(4, 'StartMenuForumShortcut', Info.PackageName .. ' Forum', Info.Forum) .. [[
			 </Directory>
		</Directory>

		<Feature Id="Core" Level="1">
			<ComponentRef Id="CoreComponent" />
			]] .. PackageBinaryFeatureItems() .. [[
			<ComponentRef Id="CoreLicense" />
			<ComponentRef Id="CoreWebsiteShortcut" />
			<ComponentRef Id="CoreForumShortcut" />
			]] .. PackageSourceFeatureItems() .. [[
			<ComponentRef Id="ApplicationShortcuts" />
			<ComponentRef Id="StartMenuWebsiteShortcut" />
			<ComponentRef Id="StartMenuForumShortcut" />
		</Feature>
	</Product>
</Wix>
]]):close()

