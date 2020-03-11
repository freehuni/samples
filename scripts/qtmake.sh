#-------------------------------------------------------------------------
# 2013/7/24 mhkang2@humaxdigital.com
# auto creation shell script
#-------------------------------------------------------------------------

if [ $# -ne 2 ]; then
	echo 'Usage: qtmake.sh <target> <type>'
	echo 'type=lib, console,...'
	exit
fi


QT_PRO=$1."pro"

echo "#========================================================="	> $QT_PRO
echo "# 2013/7/24 mhkang2@humaxdigital.com"					>> $QT_PRO
echo "# Qt Creator project file"								>> $QT_PRO
echo "#========================================================="	>> $QT_PRO

echo 'QT -= core gui'  			>> $QT_PRO
echo TARGET = $1					>> $QT_PRO

if [ $2 = 'console' ]; then
	echo 'CONFIG += console'		>> $QT_PRO
	echo 'CONFIG += qt'			>> $QT_PRO
	echo 'TEMPLATE = app'				>> $QT_PRO
else
	echo TEMPLATE = $2				>> $QT_PRO
fi

echo \									>> $QT_PRO

echo "#QMAKE_CFLAGS += -0s"	>> $QT_PRO
echo "#QMAKE_CFLAGS += -Wall"	>> $QT_PRO

echo DEFINES += '_LARGEFILE64_SOURCE=1'	>> $QT_PRO
echo DEFINES += 'SIZE_64BIT_SUPPORT '		>> $QT_PRO
echo DEFINES += '_LARGEFILE_SOURCE'		>> $QT_PRO
echo DEFINES += '_FILE_OFFSET_BITS=64'	>> $QT_PRO
echo DEFINES += '_POSIX'						>> $QT_PRO
echo "#DEFINES += more..."					>> $QT_PRO

echo \									>> $QT_PRO

for FILE in `find . -name '*.c'` ;
do
	echo 'SOURCES += $$PWD/'$FILE 		>> $QT_PRO
done

for FILE in `find . -name '*.inc'` ;
do
	echo 'SOURCES += $$PWD/'$FILE 		>> $QT_PRO
done

for FILE in `find . -name '*.cpp'` ;
do
	echo 'SOURCES += $$PWD/'$FILE 		>> $QT_PRO
done

for FILE in `find . -name '*.cc'` ;
do
	echo 'SOURCES += $$PWD/'$FILE 		>> $QT_PRO
done

for FILE in `find . -name '*.java'` ;
do
	echo 'SOURCES += $$PWD/'$FILE 		>> $QT_PRO
done

for FILE in `find . -name '*.txt'` ;
do
	echo 'SOURCES += $$PWD/'$FILE 		>> $QT_PRO
done

echo \									>> $QT_PRO

for FILE in `find . -name '*.h'` ;
do
	echo 'HEADERS += $$PWD/'$FILE			>> $QT_PRO
done

for FILE in `find . -name '*.hh'` ;
do
	echo 'HEADERS += $$PWD/'$FILE			>> $QT_PRO
done

for FILE in `find . -name '*.hpp'` ;
do
	echo 'HEADERS += $$PWD/'$FILE			>> $QT_PRO
done

for FILE in `find . -name '*.js'` ;
do
	echo 'HEADERS += $$PWD/'$FILE			>> $QT_PRO
done

echo \									>> $QT_PRO

echo 'INCLUDEPATH += $$PWD/..'					>> $QT_PRO

echo '#LIBS += -lpthread' >> $QT_PRO

echo $QT_PRO was created!
