관련 ALM : http://alm.humaxdigital.com/cb/issue/1756516

빌드방법:

* library
	igdclient/cmake.build $> cmake ..
	igdclient/cmake.build $> make install

	-DCMAKE_BUILD_TYPE=debug | reelase (이 옵션을 사용하지 않으면 default release으로 빌드됨)
	ex) cmake -DCMAKE_BUILD_TYPE=debug ..

* sample
 	igdclient/sample $> cmake .
 	igdclient/sample $> make
  	igdclient/sample $> igdclient (실행)

