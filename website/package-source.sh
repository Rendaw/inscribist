#!/bin/bash
Date=$(date +"%Y-%j")
SourcePackage=inscribist-$Date.tar.bz2

rm $SourcePackage
SourceFiles=$(find ../ -type f -name "Tupfile" -o -name "Tuprules.tup" -o -name "*.cxx" -o -name "*.h" | xargs)
tar jcvf inscribist/$SourcePackage ../LICENSE.txt $SourceFiles ../data

SettingsFile=inscribist/settings.php
echo "<?php" > $SettingsFile
echo "\$SourcePackageDate = \""$Date"\";" >> $SettingsFile
echo "\$SourcePackageFilename = \""$SourcePackage"\";" >> $SettingsFile
