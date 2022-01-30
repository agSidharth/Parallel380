all:
	g++ -fPIC -g -c -Wall psort.cpp -fopenmp
	g++ -shared -Wl,-soname,libsort.so -o libsort.so psort.o -lc

driver:
	g++ -fopenmp -Wall psort.cpp driver.cpp -o out
clean:
	rm *.so *.o