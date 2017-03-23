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
#include <condition_variable>
#include <algorithm>
#include <chrono>

// include c modules
#include <time.h>
#include <csignal>

// include custom c++ function files
#include "curlsingle.h"
#include "parse.h"
#include "parsesite.h"

// namespace declaration
using namespace std;

// data structure for queues
struct fetch_data
{
	int run_num;
	string source;
};
struct parse_data
{
	time_t fetchtime;
	string source;
	string body;
	int run_num;
};

// create global vectors
vector<string> search_terms;
vector<string> sites;

// create global queues
queue<fetch_data> fetches;
queue<parse_data> parses;

// create global mutexes
mutex m_fetches;
mutex m_parses;
mutex m_results;

// create global condition variables
condition_variable cv_fetches;
condition_variable cv_parses;

bool file_exists(string filename)
{
	/* validates existence of file */
	ifstream infile(filename);
	return infile.good();
}

void fetch_thread_function()
{
	/* function to control a fetch thread */
	
	// continue loop until program ends
	while (1)
	{
		// lock m_fetches mutex
		unique_lock<mutex> f_lock(m_fetches);
		fetch_data src;
		// get site from fetches queue
		cv_fetches.wait(f_lock, []{return !fetches.empty();});
		src = fetches.front();
		fetches.pop();
		// unlock m_fetches mutex
		f_lock.unlock();
		// record time curl commences
		time_t f_time;
		time(&f_time);
		// download site data
		string body = curl_url(src.source);
		// check curl_url() result for errors (timeout)
		while (body.compare("failure") == 0)
		{
			body = curl_url(src.source);
		}
		// create data object for parses queue
		parse_data d;
		d.fetchtime = f_time;
		d.source = src.source;
		d.body = body;
		d.run_num = src.run_num;
		// lock m_parses mutex
		unique_lock<mutex> p_lock(m_parses);
		// push data object to parses queue
		parses.push(d);
		// signal on condition variable for m_parses
		cv_parses.notify_one();
		// unlock m_parses mutex
		p_lock.unlock();
	}
}

void parse_thread_function()
{
	/* function to control a parse thread */
	
	// continue loop until program ends
	while (1)
	{
		// lock m_parses mutex
		unique_lock<mutex> p_lock(m_parses);
		parse_data db;
		// get data from parses queue
		cv_parses.wait(p_lock, []{return !parses.empty();});
		db = parses.front();
		parses.pop();
		// unlock m_parses mutex
		p_lock.unlock();
		// parse data for each search term
		for (string target : search_terms)
		{
			// count occurences of term
			int count = count_occurrences(db.body, target);
			// format lookup time
			struct tm * datetm = gmtime(&(db.fetchtime));
			string timedate(asctime(datetm));
			timedate.erase(remove(timedate.begin(),timedate.end(), '\n'), timedate.end());
			// lock results mutex
			unique_lock<mutex> r_lock(m_results);
			// create stream to output file
			ofstream outfile;
			string filename = to_string(db.run_num) + ".csv";
			outfile.open(filename, ofstream::out | ofstream::app);
			// output results
			outfile << timedate << "," << target << "," << db.source << "," << count << endl;
			// close output file
			outfile.close();
			// unlock results mutex
			r_lock.unlock();
		}
		
	}
}

void signalHandler( int signum )
{
	/* exits gracefully after current queues have been emptied */
	
	// let queues empty
	while (!fetches.empty())
	{
		continue;
	}
	while (!parses.empty())
	{
		continue;
	}
	// sleep for 1 second to ensure parse threads are finished
	this_thread::sleep_for(chrono::milliseconds(1));
	// ensure no files are being edited
	unique_lock<mutex> r_lock(m_results);
	
	exit(0);
}

int main( int argc, char * argv[] )
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
	
	// ensure specified search and sites files exist
	if (!file_exists(searf))
	{
		cerr << "Error: " << searf << " is an infalid file" << endl;
		exit(1);
	}
	if (!file_exists(sitf))
	{
		cerr << "Error: " << sitf << " is an infalid file" << endl;
		exit(1);
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
	
	/* --------------- catch interrupt signals to exit gracefully --------------- */
	
	signal(SIGINT, signalHandler);
	signal(SIGHUP, signalHandler);
	
	/* --------------- fill queue every (per) seconds --------------- */
	
	// create timer variables
	time_t old_time;
	time_t c_time;
	// fill queue
	fetch_data fd;
	int rn = 1;
	// lock results mutex
	unique_lock<mutex> r_lock(m_results);
	// create stream to output file
	ofstream outfile;
	string filename = to_string(rn) + ".csv";
	outfile.open(filename, ofstream::out | ofstream::app);
	// output results
	outfile << "Time,Phrase,Site,Count" << endl;
	// close output file
	outfile.close();
	// unlock results mutex
	r_lock.unlock();
	for (string source : sites)
	{
		fd.source = source;
		fd.run_num = rn;
		fetches.push(fd);
		// signal condition variable
		cv_fetches.notify_one();
	}
	// get time
	time(&old_time);
	// loop to continue filling queue until program ends
	while (1)
	{
		// get current time
		time(&c_time);
		// fill queue again if (per) seconds has passed
		if (difftime(c_time,old_time) > (float)per)
		{
			// increment run number
			rn++;
			// lock results mutex
			r_lock.lock();
			// create stream to output file
			ofstream outfile;
			string filename = to_string(rn) + ".csv";
			outfile.open(filename, ofstream::out | ofstream::app);
			// output results
			outfile << "Time,Phrase,Site,Count" << endl;
			// close output file
			outfile.close();
			// unlock results mutex
			r_lock.unlock();
			// fill queue
			for (string source : sites)
			{
				fd.source = source;
				fd.run_num = rn;
				fetches.push(fd);
				// signal condition variable
				cv_fetches.notify_one();
			}
			// get time
			time(&old_time);
		}
	}
	
	return 0;
}
