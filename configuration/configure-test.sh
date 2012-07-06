echo flag debug
echo
echo platform
echo
echo install-data-directory inscribist
echo
echo install-executable-directory inscribist
echo
echo c++-compiler c++11
echo
echo c++-library lua
echo
echo c++-library bz2
echo
echo c++-library gtk ">2.2" "<3.0"
echo

echo > configure-test.txt
while read line;
do
	echo Line: $line >> configure-test.txt
done

