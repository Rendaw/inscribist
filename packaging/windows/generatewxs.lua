#!/usr/bin/lua
dofile '../../info.include.lua'

local UpgradeGUID = '628A99DD-AEE3-423E-B949-4781D97D98B5'
local PackageGUID = 'E2D0EB03-D081-4BFD-B2BD-87BB8F5EFDAC'
local Version = '1.' .. tostring(Info.Version) .. '.0'

io.open('./install_' .. Info.PackageName .. '.wxs', 'w+'):write([[
<?xml version="1.0"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
	<Product Id="*" UpgradeCode="]] .. UpgradeGUID .. [[" Name="]] .. Info.PackageName .. [[" Version="]] .. Version .. [[" Manufacturer="]] .. Info.Company .. [[" Language="1033">
		<Package InstallerVersion="200" Compressed="yes" Comments="Windows Installer Package" />
		<Media Id="1" Cabinet="product.cab" EmbedCab="yes" />
		<Icon Id="Icon32" SourceFile="..\..\data\icon32.ico" />
		<Property Id="ARPPRODUCTICON" Value="Icon32" />
		<Property Id="ARPHELPLINK" Value="]] .. Info.Website .. [[" />
		<Property Id="ARPURLINFOABOUT" Value="]] .. Info.CompanyWebsite .. [[" />

		<Directory Id="TARGETDIR" Name="SourceDir">
			 <Directory Id="ProgramFilesFolder">
				<Directory Id="INSTALLDIR" Name="]] .. Info.PackageName .. [[">
					<Component Id="CoreComponent" Guid="*">
						<File Id="CoreComponentFile" Source="..\..\app\build\inscribist.exe" KeyPath="yes" Checksum="yes" />
					</Component>
				</Directory>
			 </Directory>

			 <Directory Id="ProgramMenuFolder">
				<Component Id="ApplicationShortcuts" Guid="*">
					<Shortcut Id="ApplicationShortcutFile" Name="]] .. Info.PackageName .. [[" Target="[INSTALLDIR]inscribist.exe" WorkingDirectory="INSTALLDIR" Icon="Icon32" />
					<RegistryValue Root="HKCU" Key="Software\]] .. Info.Company .. [[\]] .. Info.PackageName .. [[" Name="installed" Type="integer" Value="1" KeyPath="yes"/>
				</Component>
			 </Directory>
		</Directory>

		<Feature Id="Core" Level="1">
			<ComponentRef Id="CoreComponent" />
			<ComponentRef Id="ApplicationShortcuts" />
		</Feature>
	</Product>
</Wix>
]]):close()

