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

if [ $DEBUG ]
then
	echo c++ compile-object c++11 debug nooptimize
	echo
	if [ ! $HELP ]
	then
		read CompileObject
		ReadRest
	fi

	echo c++ compile-object input
	echo
	if [ ! $HELP ]
	then
		read CompileObjectInput
		ReadRest
	fi

	echo c++ compile-object output
	echo
	if [ ! $HELP ]
	then
		read CompileObjectOutput
		ReadRest
	fi

	echo CONFIG_GENERATEOBJECT=$CompileObject $CompileObjectInput %f $CompileObject
else
fi

