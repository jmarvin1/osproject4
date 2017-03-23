all: curlsingle.o

curlsingle.o: curlsingle.cpp curlsingle.h
	g++ -std=gnu++11 -static-libstdc++ -Wall -lcurl -c curlsingle.cpp

clean:
	rm *.o
