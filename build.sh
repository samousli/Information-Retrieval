# creates a cmake file, builds the app and the runs it.
cd build 
cmake -G Unix\ Makefiles ../src
make
mv IR ../
cd ..
./IR

#clang++  -pedantic-errors -std=c++11 -pthread -g -O3 -Wall \
#	ConcurrentQueue.hpp SysUtils.hpp VectorOps.hpp IndexStat.hpp \
#	 IndexVector.hpp Logger.hpp InvertedIndex.hpp QueryExecutor.hpp main.cpp 
#mv a.out IR.clang.out
# g++  -pedantic-errors -std=c++11 -pthread -g -O3 -Wall \
#	ConcurrentQueue.hpp SysUtils.hpp VectorOps.hpp IndexStat.hpp \
#	 IndexVector.hpp Logger.hpp InvertedIndex.hpp QueryExecutor.hpp main.cpp -o IR.gcc.out
#gcc -ftrapv  -pedantic-errors -Wall -std=c++11 InvertedIndex.cpp

#valgrind --dsymutil=yes --tool=callgrind ./a.out 
