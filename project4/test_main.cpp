#include <iostream>
#include <string>
#include "curlsingle.h"

int main() {
	string r;
	r = curl_url("http://www.nd.edu");
	cout << r << endl;
	return 0;
}
