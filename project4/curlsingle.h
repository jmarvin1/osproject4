//curlsingle.hpp

#include <string>

using namespace std;

size_t CurlSingleWriteFunction(char *contents, size_t size, size_t 
nmenb, string *resultsptr);

string curl_url(string url);
