#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include "parse.h"
using namespace std;

/*int main(int argc, char* argv[])
{
	ifstream infile(argv[1]);
	string line,delimeter;
	string period = "PERIOD_FETCH";
	string numf = "NUM_FETCH";
	string nump = "NUM_PARSE";
	string sf = "SEARCH_FILE";
	string sif = "SITE_FILE";
	int per,nf,np;
	string searf,sitf;
	while(getline(infile,line))
	{
		delimeter="=";
		if(period.compare(line.substr(0,line.find(delimeter)))==0)
		{
			//cout<<line.substr(line.find(delimeter)+1)<<endl;
			per=stoi(line.substr(line.find(delimeter)+1));
			cout<<per;
		}
		else if(numf.compare(line.substr(0,line.find(delimeter)))==0)
		{
			nf=stoi(line.substr(line.find(delimeter)+1));
			cout<<nf;
		}
		else if(nump.compare(line.substr(0,line.find(delimeter)))==0)
		{
			np=stoi(line.substr(line.find(delimeter)+1));
			cout<<np;
		}
		else if(sf.compare(line.substr(0,line.find(delimeter)))==0)
		{
			searf=line.substr(line.find(delimeter)+1);
			cout<<searf;
		}
		else if(sif.compare(line.substr(0,line.find(delimeter)))==0)
		{
			sitf=line.substr(line.find(delimeter)+1);
			cout<<sitf;
		}
	}
	cout<<endl;
	
	vector<string> test;
	test=parseFile(searf);
	
	for(int i=0; i<test.size(); i++)
	{
		cout<<test[i];
	}
	cout<<endl;
	
	vector<string> test1;
	test1=parseFile(sitf);
	for(int i=0; i<test1.size(); i++)
	{
		cout<<test1[i];
	}
	cout<<endl;
	

}*/

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


