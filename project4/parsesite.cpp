//parsesite.cpp
#include <iostream>
#include <string>

using namespace std;

int count_occurrences(string site, string search) {
	int occurrences = 0;
	string::size_type pos = 0;
	while ((pos = site.find(search, pos)) != string::npos) {
		occurrences++;
		pos += 1;
	}
	
	return occurrences;
}
