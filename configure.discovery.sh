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
	if [ $DEBUG ]; then echo CONFIG_DEBUG=true >> tup.config; fi
	echo CONFIG_ROOT=$ROOTDIR >> tup.config
	echo CONFIG_VERSION=$PROJECTVERSION >> tup.config
	echo CONFIG_CFLAGS=$CFLAGS >> tup.config

	echo > $ROOTDIR/installsettings.sh
fi

# Start gathering build information
echo install-data-directory inscribist
echo
if [ ! $HELP ]
then
	read Location Extra
	echo DATADIRECTORY=$Location >> $ROOTDIR/installsettings.sh
	echo CONFIG_DATADIRECTORY=$Location >> tup.config
	ReadRest
fi

echo install-executable-directory inscribist
echo
if [ ! $HELP ]
then
	read Location Extra
	echo EXECUTABLEDIRECTORY=$Location >> $ROOTDIR/installsettings.sh
	ReadRest
fi

echo platform
echo
if [ ! $HELP ]
then
	read Family Iteration Extra
	case "$family" in
		windows) echo CONFIG_PLATFORM=windows >> tup.config ;;
		*) echo CONFIG_PLATFORM=linux >> tup.config ;;
	esac
	ReadRest
fi

echo c++-compiler c++11
echo
if [ ! $HELP ]
then
	read Compiler FullPath Extra
	echo CONFIG_COMPILERNAME=$Compiler >> tup.config
	echo CONFIG_COMPILER=$FullPath >> tup.config
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

echo -n CONFIG_LIBDIRECTORIES=>> tup.config
sort -u <(echo -e $BuildLibraryDirectories) | while read line; do echo -n " -L$line" >> tup.config; done
echo >> tup.config
echo -n CONFIG_INCLUDEDIRECTORIES=>> tup.config
sort -u <(echo -e $BuildIncludeDirectories) | while read line; do echo -n " -I$line" >> tup.config; done
echo >> tup.config
echo CONFIG_LIBRARIES=$Libraries >> tup.config

