#include <NSCpp.h>
#include <iostream>

using namespace std;

int main() {
  	// Create API object, initialize it with an useragent
	NSCpp::API api("Explanation of what your script does", "Nation belonging to the script's author", "Nation belonging to the script's user");

 	// Request population shard from the nation named testnation, returns a struct containing requested shard, request type and API response
	NSCpp::Shard response = api.APIRequest("nation", "population", "testnation");

  	// Print the API response
	cout << response.response;

	// Request happenings shard from world
	NSCpp::Shard response2 = api.APIRequest("world", "happenings");

	// Some shards like happenings return multiple entries of data, in that case, a vector of maps will be returned through the responseSpecial field
	// NSCpp::Mapvec is short-hand for std::vector<std::map<std::string, std::string>>
	NSCpp::Mapvec vectorOfMaps = response2.responseSpecial;

	// Iterate over the vector and print data
	for(map<string, string> currentEvent : vectorOfMaps){
		cout << "Text: " << currentEvent["TEXT"] << "\nTimestamp: " << currentEvent["TIMESTAMP"] << "\n\n";
	}
	return 0;
}
