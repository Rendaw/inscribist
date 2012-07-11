#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

# Read initial configuration
while read line
do
	if [ -z "$line" ]; then break; fi
	case "$line" in
		help) HELP=set ;;
	esac
done

# Set up various things
if [ ! $HELP ]
then
	$(cd .. && tup init)
	echo > tup.config # Clear the config file
fi

VERSION=5
if [ ! $HELP ]
then
	echo CONFIG_VERSION=$VERSION > tup.config
fi

# Auxiliary methods
function ReadRest
{
	while read line
	do
		if [ -z $line ]; then break; fi
	done
}

# Start requesting information
# ...
echo flag debug
echo
if [ ! $HELP ]
then
	read Debug Extra
	if [ "$Debug" -eq "true" ]
	then
		echo CONFIG_DEBUG=true >> tup.config
		DEBUG=set
	fi
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

echo install-data-directory inscribist
echo
if [ ! $HELP ]
then
	read Location Extra
	echo CONFIG_DATADIRECTORY=$Location >> tup.config
	ReadRest
fi

echo install-executable-directory inscribist
echo
if [ ! $HELP ]
then
	read Location Extra
	echo CONFIG_EXECUTABLEDIRECTORY=$Location >> tup.config
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
		Libraries="$Libraries -l$1"

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

