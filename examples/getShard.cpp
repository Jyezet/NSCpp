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

	// Some shards like happenings return multiple, named entries of data, in that case, a vector of maps will be returned through the respMap field
	// NSCpp::Mapvec is short-hand for std::vector<std::map<std::string, std::string>>
	NSCpp::Mapvec vectorOfMaps = response2.respMap;

	// Iterate over the vector and print data
	for(map<string, string> currentEvent : vectorOfMaps){
		cout << "Text: " << currentEvent["TEXT"] << "\nTimestamp: " << currentEvent["TIMESTAMP"] << "\n\n";
	}

	// Request banners shard from a nation named testnation2
	NSCpp::Shard response3 = api.APIRequest("nation", "banners", "testnation2")
	
	// Some shards like banners return multiple, unnamed entries of data, in that case, a vector of strings will be returned through the respVec field
	// NSCpp::Strvec is short-hand for std::vector<std::string>
	NSCpp::Strvec vectorOfStrings = response3.respVec;

	// Iterate over the vector and print data
	for(auto i : vectorOfStrings){
		cout << i << "\n";
	}
	return 0;
}
