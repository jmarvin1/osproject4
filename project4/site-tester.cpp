//site-tester.cpp
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>

#include <time.h>

#include "curlsingle.h"
#include "parse.h"
#include "parsesite.h"

using namespace std;

struct parse_data
{
	time_t fetchtime;
	string source;
	string body;
};

// create vectors
vector<string> search_terms;
vector<string> sites;

// create queues
queue<string> fetches;
queue<parse_data> parses;

// create mutexes
mutex m_fetches;
mutex m_parses;

void fetch_thread_function()
{
	unique_lock<mutex> f_lock(m_fetches);
	string source = " ";
	while (strcmp(source," ") == 0)
	{
		// wait on condition variable on m_fetches
		source = fetches.pop();
	}
	f_lock.unlock();
	time_t f_time;
	time(&f_time);
	string body = curl_url(source);
	parse_data d;
	d.fetchtime = f_time;
	d.source = source;
	d.body = body;
	unique_lock<mutex> p_lock(m_parses);
	parses.push(d);
	// signal on condition variable for m_parses
	p_lock.unlock();
}

void parse_thread_function()
{
	
}

int main( int argc, char * argv[])
{
	// validate command line input
	if(argc != 2)
	{
		cerr << "Error: Incorrect Arguments" << endl;
		cerr << "Usage:" << endl << "\t./site-tester config-file";
		return 1;
	}
	
	/* ---------- parse for input data ---------- */
	ifstream infile(argv[1]);
	string line,delimeter;
	string period = "PERIOD_FETCH";
	string numf = "NUM_FETCH";
	string nump = "NUM_PARSE";
	string sf = "SEARCH_FILE";
	string sif = "SITE_FILE";
	int per = 180; //seconds
	int nf = 1;
	int np = 1;
	string searf = "Search.txt";
	string sitf = "Sites.txt";
	while(getline(infile,line))
	{
		delimeter="=";
		if(period.compare(line.substr(0,line.find(delimeter)))==0)
		{
			per=stoi(line.substr(line.find(delimeter)+1));
			
			// enforce sensible input
			if (per <= 0 || per > 500)
			{
				per = 180;
			}
		}
		else if(numf.compare(line.substr(0,line.find(delimeter)))==0)
		{
			nf=stoi(line.substr(line.find(delimeter)+1));
			
			// enforce sensible input
			if (nf <= 0 || nf > 100)
			{
				nf = 1;
			}
		}
		else if(nump.compare(line.substr(0,line.find(delimeter)))==0)
		{
			np=stoi(line.substr(line.find(delimeter)+1));
			
			// enforce sensible input
			if (np <= 0 || nf > 100)
			{
				np = 1;
			}
		}
		else if(sf.compare(line.substr(0,line.find(delimeter)))==0)
		{
			searf=line.substr(line.find(delimeter)+1);
		}
		else if(sif.compare(line.substr(0,line.find(delimeter)))==0)
		{
			sitf=line.substr(line.find(delimeter)+1);
		}
	}
	
	// parse search terms file
	search_terms=parseFile(searf);
	
	// parse sites file
	sites=parseFile(sitf);
	
	/* ---------- create threads ---------- */
	
	// fetch threads
	thread fetch_threads[nf];
	for (int i=0; i < nf; i++)
	{
		fetch_threads[i] = thread(fetch_thread_function);
	}
	
	// parse threads
	thread parse_threads[np];
	for (int i=0; i<np; i++)
	{
		parse_threads[i] = thread(parse_thread_function);
	}
	
	/* ---------- fill queue every (per) seconds ---------- */
	
	// create timer variables
	time_t old_time;
	time_t c_time;
	
	// fill queue
	for (string source : sites)
	{
		fetches.push(source);
		// signal condition variable
		// ------------------------------------------------------------------------
	}
	// get time
	time(&old_time)
	
	// loop to continue filling queue
	while (1)
	{
		// get current time
		time(&c_time);
		// fill queue again if (per) seconds has passed
		if (difftime(c_time,old_time) > (float)per)
		{
			// fill queue
			for (string source : sites)
			{
				fetches.push(source);
				// signal condition variable
				// --------------------------------------------------------------------------
			}
			// get time
			time(&old_time)
		}
	}
	
	/* ---------- catch interrupts and join threads again ---------- */
	
	return 0;
}
