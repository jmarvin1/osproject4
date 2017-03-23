all: curlsingle.o parsesite.o

curlsingle.o: curlsingle.cpp curlsingle.h
	g++ -std=gnu++11 -static-libstdc++ -Wall -lcurl -c curlsingle.cpp

parsesite.o: parsesite.cpp parsesite.h
	g++ -std=gnu++11 -static-libstdc++ -Wall -c parsesite.cpp

clean:
	rm *.o
