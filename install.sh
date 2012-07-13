#!/bin/bash
if [ ! -d variant-release ]
then
	echo Note: This script only installs release executables.  If you built the binary in debug mode, you will have to install it manually. 1>&2
fi

. installsettings.sh
mkdir -p $EXECUTABLEDIRECTORY
cp variant-release/build/inscribist $EXECUTABLEDIRECTORY
mkdir -p $DATADIRECTORY
cp -r data/* $DATADIRECTORY

