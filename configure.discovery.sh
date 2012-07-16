#!/bin/bash
ROOTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

PROJECTVERSION=5

# Read initial configuration, verify discovery command version
function ReadRest
{
	while read line
	do
		if [ -z $line ]; then break; fi
	done
}

while read line
do
	if [ -z "$line" ]; then break; fi
	case "$line" in
		help) HELP=set ;;
	esac
done

echo version 0
echo
if [ ! $HELP ]; then ReadRest; fi

# Prepare directories for installation
echo flag cleanconfigs "Deletes configuration-generated files.  Note: if you reconfigure, these files will be entirely overwritten, so it is not necessary to do this to reconfigure."
echo
if [ ! $HELP ]
then
	read Clean Extra
	ReadRest
	if [ "$Clean" == "true" ]
	then
		rm -r variant-debug
		rm -r variant-release
		rm installsettings.sh
		exit 0
	fi
fi

VARIANTDIR=$ROOTDIR/variant-release
echo flag debug "Enables building a debug executable.  The debug executable will be built in variant-debug/."
echo
if [ ! $HELP ]
then
	read Debug Extra
	ReadRest
	if [ "$Debug" == "true" ]
	then
		VARIANTDIR=$ROOTDIR/variant-debug
		DEBUG=set
	fi
fi

if [ ! $HELP ]
then
	if [ ! -d $VARIANTDIR ]; then mkdir $VARIANTDIR; fi
	cd $VARIANTDIR

	if [ ! -d $ROOTDIR/.tup ]; then $(cd $ROOTDIR && tup init); fi

	echo > tup.config
	if [ ! $DEBUG ]; then echo > $ROOTDIR/installsettings.sh; fi
fi

function WriteTupConfig
{
	echo $1 >> tup.config
}

function WriteInstallConfig
{
	if [ ! $DEBUG ]; then echo $1 >> $ROOTDIR/installsettings.sh; fi
}

if [ ! $HELP ]
then
	if [ $DEBUG ]; then WriteTupConfig "CONFIG_DEBUG=true"; fi
	WriteTupConfig "CONFIG_ROOT=$ROOTDIR"
	WriteTupConfig "CONFIG_VERSION=$PROJECTVERSION"
	WriteTupConfig "CONFIG_CFLAGS=$CFLAGS"
fi

# Start gathering build information
if [ ! $DEBUG ]
then
	echo install-data-directory inscribist
	echo
	if [ ! $HELP ]
	then
		read Location Extra
		WriteInstallConfig "DATADIRECTORY=$Location"
		WriteTupConfig "CONFIG_DATADIRECTORY=$Location"
		ReadRest
	fi
else
	if [ ! $HELP ]
	then
		WriteTupConfig "CONFIG_DATADIRECTORY=$ROOTDIR/data"
	fi
fi

echo install-executable-directory inscribist
echo
if [ ! $HELP ]
then
	read Location Extra
	WriteInstallConfig "EXECUTABLEDIRECTORY=$Location"
	ReadRest
fi

echo platform
echo
if [ ! $HELP ]
then
	read Family Iteration Extra
	case "$family" in
		windows) WriteTupConfig "CONFIG_PLATFORM=windows" ;;
		*) WriteTupConfig "CONFIG_PLATFORM=linux" ;;
	esac
	ReadRest
fi

echo c++-compiler c++11
echo
if [ ! $HELP ]
then
	read Compiler FullPath Extra
	WriteTupConfig "CONFIG_COMPILERNAME=$Compiler"
	WriteTupConfig "CONFIG_COMPILER=$FullPath"
	ReadRest
fi

function GetLibrary
{
	echo c-library $1
	echo
	if [ ! $HELP ]
	then
		read Filename LibraryDirectory IncludeDirectory Extra
		LibraryName=$(echo $Filename | sed -e "s/^lib//" -e "s/\.so\(\.[0-9]\+\)*//g")
		if [ -z "$BuildLibraryDirectories" ]
		then
			BuildLibraryDirectories=$LibraryDirectory
		else
			BuildLibraryDirectories="$BuildLibraryDirectories\n$LibraryDirectory"
		fi
		if [ -z "$BuildIncludeDirectories" ]
		then
			BuildIncludeDirectories=$IncludeDirectory
		else
			BuildIncludeDirectories="$BuildIncludeDirectories\n$IncludeDirectory"
		fi
		Libraries="$Libraries -l$LibraryName"

		ReadRest
	fi
}

GetLibrary lua
GetLibrary bz2
GetLibrary gtk-x11-2.0
GetLibrary gdk-x11-2.0
GetLibrary atk-1.0
GetLibrary gio-2.0
GetLibrary pangoft2-1.0
GetLibrary pangocairo-1.0
GetLibrary gdk_pixbuf-2.0
GetLibrary cairo
GetLibrary pango-1.0
GetLibrary freetype
GetLibrary fontconfig
GetLibrary gobject-2.0
GetLibrary glib-2.0

if [ ! $HELP ]
then
	WriteTupConfig $(
		echo -n CONFIG_LIBDIRECTORIES=
		sort -u <(echo -e $BuildLibraryDirectories) | while read line; do echo -n " -L$line"; done
	)
	WriteTupConfig $(
		echo -n CONFIG_INCLUDEDIRECTORIES=
		sort -u <(echo -e $BuildIncludeDirectories) | while read line; do echo -n " -I$line"; done
	)
	WriteTupConfig "CONFIG_LIBRARIES=$Libraries"
fi
