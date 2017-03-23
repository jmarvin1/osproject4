#include <iostream>
#include <string>

#include <curl/curl.h>

using namespace std;

size_t WriteFunction(char *contents, size_t size, size_t nmemb, string *resultptr) {
	for (int c = 0; c<size*nmemb; c++) {
		resultptr->push_back(contents[c]);
	}
	
	return size*nmemb;
}

int main(void)
{
	CURL *curl_handle;
	CURLcode res;
 
	string result;
 
	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */ 
	curl_handle = curl_easy_init();

	/* specify URL to get */ 
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.nd.edu/");

	/* send all data to this function  */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteFunction);

	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &result);

	/* some servers don't like requests that are made without a user-agent
	 field, so we provide one */ 
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */ 
	res = curl_easy_perform(curl_handle);

	/* check for errors */ 
	if(res != CURLE_OK) {
		cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
	}
	else {
		cout << endl << result << endl;
	}
 
	/* cleanup curl stuff */ 
	curl_easy_cleanup(curl_handle);
	 
	/* we're done with libcurl, so clean it up */ 
	curl_global_cleanup();
 
	return 0;
}