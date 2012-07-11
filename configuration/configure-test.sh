echo > configure-test.txt

ReadRest ()
{
	echo Starting reading response-- >> configure-test.txt
	while read line
	do
		echo Line: $line >> configure-test.txt
		if [ -z "$line" ]; then break; fi
	done
	echo Finished reading response-- >> configure-test.txt
}

ReadRest

echo flag debug \"Compiles with debug symbols and no optimization.\"
echo
ReadRest

echo platform
echo
ReadRest

echo install-data-directory inscribist
echo
ReadRest

echo install-executable-directory inscribist
echo
ReadRest

echo c++-compiler c++11
echo
ReadRest

echo c-library lua
echo
ReadRest

echo c-library bz2
echo
ReadRest

echo c-library gtk-x11-2.0
echo
ReadRest

echo c-library gdk-x11-2.0
echo
ReadRest

echo c-library atk-1.0
echo
ReadRest

echo c-library gio-2.0
echo
ReadRest

echo c-library pangoft2-1.0
echo
ReadRest

echo c-library pangocairo-1.0
echo
ReadRest

echo c-library gdk_pixbuf-2.0
echo
ReadRest

echo c-library cairo
echo
ReadRest

echo c-library pango-1.0
echo
ReadRest

echo c-library freetype
echo
ReadRest

echo c-library fontconfig
echo
ReadRest

echo c-library gobject-2.0
echo
ReadRest

echo c-library glib-2.0
echo
ReadRest

