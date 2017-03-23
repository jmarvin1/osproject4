/*
	site-tester.cpp
	J. Patrick Lacher	and		James Marvin
	jlacher1@nd.edu				jmarvin1@nd.edu
	Operating Systems CSE 30341
	Project IV - System for Verifying Web Placement
*/

// include c++ modules
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>

// include c modules
#include <time.h>

// include custom c++ function files
#include "curlsingle.h"
#include "parse.h"
#include "parsesite.h"

// namespace declaration
using namespace std;

// data structure for parses queue
struct parse_data
{
	time_t fetchtime;
	string source;
	string body;
};

// create global vectors
vector<string> search_terms;
vector<string> sites;

// create global queues
queue<string> fetches;
queue<parse_data> parses;

// create global mutexes
mutex m_fetches;
mutex m_parses;

void fetch_thread_function()
{
	/* function to control a fetch thread */
	
	// continue loop until program ends
	while (1)
	{
		// lock m_fetches mutex
		unique_lock<mutex> f_lock(m_fetches);
		string source = " ";
		// get site from fetches queue
		while (strcmp(source," ") == 0)
		{
			// wait on condition variable on m_fetches
			// ----------------------------------------------------------------------------------
			if (!fetches.empty())
			{
				source = fetches.front();
				fetches.pop();
			}
		}
		// unlock m_fetches mutex
		f_lock.unlock();
		// record time curl commences
		time_t f_time;
		time(&f_time);
		// download site data
		string body = curl_url(source);
		// create data object for parses queue
		parse_data d;
		d.fetchtime = f_time;
		d.source = source;
		d.body = body;
		// lock m_parses mutex
		unique_lock<mutex> p_lock(m_parses);
		// push data object to parses queue
		parses.push(d);
		// signal on condition variable for m_parses
		// ----------------------------------------------------------------------------------------
		// unlock m_parses mutex
		p_lock.unlock();
	}
}

void parse_thread_function()
{
	/* function to control a parse thread */
	
	// --------------------------------------------------------------------------------------------
}

int main( int argc, char * argv[])
{
	/* main program execution */
	
	// validate number of command line inputs
	if(argc != 2)
	{
		cerr << "Error: Incorrect Arguments" << endl;
		cerr << "Usage:" << endl << "\t./site-tester config-file";
		return 1;
	}
	
	/* --------------- parse for input data --------------- */
	
	ifstream infile(argv[1]);
	string line,delimeter;
	// parameter labels
	string period = "PERIOD_FETCH";
	string numf = "NUM_FETCH";
	string nump = "NUM_PARSE";
	string sf = "SEARCH_FILE";
	string sif = "SITE_FILE";
	// default parameters
	int per = 180; // seconds between queue fills
	int nf = 1; // fetch threads
	int np = 1; // parse threads
	string searf = "Search.txt"; // search terms file
	string sitf = "Sites.txt"; // searchable sites file
	// loop over configuration file for parameters
	while(getline(infile,line))
	{
		delimeter="=";
		// period
		if(period.compare(line.substr(0,line.find(delimeter)))==0)
		{
			per=stoi(line.substr(line.find(delimeter)+1));
			// enforce sensible input
			if (per <= 0 || per > 500)
			{
				per = 180;
			}
		}
		// number of fetch threads
		else if(numf.compare(line.substr(0,line.find(delimeter)))==0)
		{
			nf=stoi(line.substr(line.find(delimeter)+1));
			// enforce sensible input
			if (nf <= 0 || nf > 100)
			{
				nf = 1;
			}
		}
		// number of parse threads
		else if(nump.compare(line.substr(0,line.find(delimeter)))==0)
		{
			np=stoi(line.substr(line.find(delimeter)+1));
			// enforce sensible input
			if (np <= 0 || nf > 100)
			{
				np = 1;
			}
		}
		// search terms file name
		else if(sf.compare(line.substr(0,line.find(delimeter)))==0)
		{
			searf=line.substr(line.find(delimeter)+1);
		}
		// searchable sites file name
		else if(sif.compare(line.substr(0,line.find(delimeter)))==0)
		{
			sitf=line.substr(line.find(delimeter)+1);
		}
	}
	
	// parse search terms file into search_terms vector
	search_terms = parseFile(searf);
	// parse sites file into sites vector
	sites = parseFile(sitf);
	
	/* --------------- create threads --------------- */
	
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
	
	/* --------------- catch interrupts to exit gracefully --------------- */
	
	// -------------------------------------------------------------------------------------------
	
	/* --------------- fill queue every (per) seconds --------------- */
	
	// create timer variables
	time_t old_time;
	time_t c_time;
	// fill queue
	for (string source : sites)
	{
		fetches.push(source);
		// signal condition variable
		// ---------------------------------------------------------------------------------------
	}
	// get time
	time(&old_time)
	// loop to continue filling queue until program ends
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
				// -------------------------------------------------------------------------------
			}
			// get time
			time(&old_time)
		}
	}
	
	return 0;
}