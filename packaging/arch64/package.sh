#!/bin/bash
if [ ! -d ../../variant-release ]
then
	echo You must have a release build in variant directory variant-release/ for this to work. 1>&2
	exit 1
fi
mkdir -p src
cp ../../variant-release/build/inscribist src
cp -r ../../data src
cp ../../LICENSE.txt src
makepkg --repackage --force

