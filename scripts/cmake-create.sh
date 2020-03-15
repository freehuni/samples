#-------------------------------------------------------------------------
# freehuni@hanmail.net
# auto creation shell script
#-------------------------------------------------------------------------

if [ $# -ne 2 ]; then
	echo 'Usage: qtmake.sh <target> <type>'
	echo 'type=lib, app'
	exit
fi


CMAKE_FILE="CMakeLists.txt"

echo "#========================================================="	> $CMAKE_FILE
echo "# Create CMakeLists.txt for cmake environment"				>> $CMAKE_FILE
echo "#========================================================="	>> $CMAKE_FILE
echo \										>> $CMAKE_FILE
echo "CMAKE_MINIMUM_REQUIRED(VERSION 2.8)"	>> $CMAKE_FILE
echo "SET(PROJECT_NAME \"$1\")"				>> $CMAKE_FILE
echo "PROJECT(\${PROJECT_NAME})"			>> $CMAKE_FILE
echo \										>> $CMAKE_FILE
echo "ADD_DEFINITIONS( -D_POSIX -D_LARGEFILE64_SOURCE=1 -DSIZE_64BIT_SUPPORT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)"	>> $CMAKE_FILE

echo \										>> $CMAKE_FILE

for FILE in `find $(pwd) -name '*.h'` ;
do
	echo "INCLUDE_DIRECTORIES($(dirname $FILE))" >>$CMAKE_FILE
	break;
done

for FILE in `find $(pwd) -name '*.hh'` ;
do
	echo "INCLUDE_DIRECTORIES($(dirname $FILE))" >>$CMAKE_FILE
	break;
done

for FILE in `find $(pwd) -name '*.hpp'` ;
do
	echo "INCLUDE_DIRECTORIES($(dirname $FILE))" >>$CMAKE_FILE
	break;
done

echo \									>> $CMAKE_FILE

if [ $2 = 'app' ]; then
	echo "ADD_EXECUTABLE(\${PROJECT_NAME}" >> $CMAKE_FILE
else
	echo "ADD_LIBRARY(\${PROJECT_NAME}" >> $CMAKE_FILE
fi

for FILE in `find . -name '*.c'` ;
do
	if [[ $FILE == *"CMakeFiles"* ]]; then
		echo "ignore path: $FILE"
	else
		echo "    "$FILE 		>> $CMAKE_FILE
	fi
done

for FILE in `find . -name '*.cpp'` ;
do
	if [[ $FILE == *"CMakeFiles"* ]]; then
		echo "ignore path: $FILE"
	else
		echo "    "$FILE 		>> $CMAKE_FILE
	fi
done

for FILE in `find . -name '*.cc'` ;
do
	if [[ $FILE == *"CMakeFiles"* ]]; then
		echo "ignore path: $FILE"
	else
		echo "    "$FILE 		>> $CMAKE_FILE
	fi
done

echo ")" >> $CMAKE_FILE

echo \									>> $CMAKE_FILE

echo "# TARGET_LINK_LIBRARIES(\${PROJECT_NAME} pthread)" >> $CMAKE_FILE

echo $CMAKE_FILE was created!
