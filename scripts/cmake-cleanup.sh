find . -name CMakeCache.txt -exec rm {} \;
find . -name cmake_install.cmake -exec rm {} \;
find . -name CMakeFiles -exec rm -rf {} \;
rm -rf Testing
