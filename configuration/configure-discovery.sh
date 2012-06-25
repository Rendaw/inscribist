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
$(cd .. && tup init)
echo > tup.config # Clear the config file

VERSION=5
echo CONFIG_VERSION=$VERSION > tup.config

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
	read debug extra
	if [ "$debug" -eq "true" ]
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
	echo CONFIG_DATADIRECTORY=$Location
	ReadRest
fi

echo install-binary-directory inscribist
echo
if [ ! $HELP ]
then
	read Location Extra
	echo CONFIG_BINDIRECTORY=$Location
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

echo c++-library lua
echo
if [ ! $HELP ]
then
	read IncludeDirectory LibraryDirectory LibraryName ShortName Extra
	echo CONFIG_LUAINCLUDEDIRECTORY=$IncludeDirectory >> tup.config
	echo CONFIG_LUALIBRARYDIRECTORY=$LibraryDirectory >> tup.config
	echo CONFIG_LUALIBRARYNAME=$ShortName >> tup.config
	ReadRest
fi

echo c++-library bz2
echo
if [ ! $HELP ]
then
	read IncludeDirectory LibraryDirectory LibraryName ShortName Extra
	echo CONFIG_BZ2INCLUDEDIRECTORY=$IncludeDirectory >> tup.config
	echo CONFIG_BZ2LIBRARYDIRECTORY=$LibraryDirectory >> tup.config
	echo CONFIG_BZ2LIBRARYNAME=$ShortName >> tup.config
	ReadRest
fi

echo c++-library gtk ">2.2" "<3.0"
echo
if [ ! $HELP ]
then
	read IncludeDirectory LibraryDirectory LibraryName ShortName Extra
	echo CONFIG_GTKINCLUDEDIRECTORY=$IncludeDirectory >> tup.config
	echo CONFIG_GTKLIBRARYDIRECTORY=$LibraryDirectory >> tup.config
	echo CONFIG_GTKLIBRARYNAME=$ShortName >> tup.config
	ReadRest
fi

