all: site-tester

site-tester: curlsingle.o parsesite.o parse.o
	g++ -std=gnu++11 -static-libstdc++ -Wall -lcurl site-tester.cpp curlsingle.o parsesite.o parse.o -o site-tester

curlsingle.o: curlsingle.cpp curlsingle.h
	g++ -std=gnu++11 -static-libstdc++ -Wall -lcurl -c curlsingle.cpp

parsesite.o: parsesite.cpp parsesite.h
	g++ -std=gnu++11 -static-libstdc++ -Wall -c parsesite.cpp

parse.o: parse.h parse.cpp
	g++ -std=gnu++11 -static-libstdc++ -Wall -c parse.cpp 

clean:
	rm *.o
	rm site-tester
