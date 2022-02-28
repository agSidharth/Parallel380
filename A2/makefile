compile:
	mpic++ -std=c++17 -o wtf *.cpp librandomizer.a
run:
	mpirun -np 4 ./wtf data/8717_31525/edges.dat 8717 31525 0.1 5000 30 20 369
run2:
	mpirun -np 4 ./wtf data/81867_545671/edges.dat 81867 545671 0.1 5000 30 20 369
run3:
	mpirun -np 4 ./wtf data/82168_870161/edges.dat 82168 870161 0.1 5000 30 20 369
clean:
	rm wtf