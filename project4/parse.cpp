// parse.cpp

#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include "parse.h"

using namespace std;

vector<string> parseFile(string filename)
{

	vector<string> searchTerms;
	ifstream infile(filename);
	string line;
	while(getline(infile,line)){
		searchTerms.push_back(line);
	}

	return searchTerms;
}


