all: curlsingle.o

curlsingle.o: curlsingle.cpp curlsingle.h
	g++ -std=gnu++11 -Wall -lcurl -c curlsingle.cpp

clean:
	rm *.o
