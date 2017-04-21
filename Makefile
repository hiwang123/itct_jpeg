all: main.cpp util.cpp decoder.cpp
	g++ -std=c++11 -O3 -o main main.cpp util.cpp decoder.cpp 
clean:
	rm -f main
