// If you find an error in this example, contact me through discord (Username: jyezet)
#include <NSCpp.h>
#include <iostream>
#include <string>

using namespace std;

int main() {
  	// Create API object, initialize it with an useragent
	NSCpp::API api("Explanation of what your script does", "Nation belonging to the script's author", "Nation belonging to the script's user");

 	// Request population shard from the nation named testnation, returns a struct containing requested shard, request type and API response
	NSCpp::Shard response = api.APIRequest("nation", "population", "testnation");

  	// Print the API response
	cout << response.response;

	// Request census shard from the nation named coolnation, sending additional attributes (In this case, specifying census scales)
	NSCpp::Shard response2 = api.APIRequest("nation", "census", "coolnation", Strvec({ "scale" }), Strvec({ NSCpp::censusTypes::BlackMarket }));

	// Some shards like census return multiple, named entries of data, in that case, a vector of maps will be returned through the respMapVec field
	// NSCpp::Mapvec is short-hand for std::vector<std::map<std::string, std::string>>
	NSCpp::Mapvec vectorOfMaps = response2.respMapVec;
	for(auto censusScale : vectorOfMaps){
		cout << "Score: " << censusScale["SCORE"] << "\nWorld rank: " << censusScale["RANK"] << "\nRegion rank: " << censusScale["RRANK"] <<"\n\n";
	}

	// You can pass additional arguments to the request URL this way
	NSCpp::Shard response2 = api.APIRequest("nation", "census", "coolnation", "&scale=45");

	// Print everything again
	NSCpp::Mapvec vectorOfMaps = response2.respMapVec;
	for(auto censusScale : vectorOfMaps){
		cout << "Score: " << censusScale["SCORE"] << "\nWorld rank: " << censusScale["RANK"] << "\nRegion rank: " << censusScale["RRANK"] <<"\n\n";
	}
	
	// Some shards like census return multiple, named entries of data, in that case, a vector of maps will be returned through the respMapVec field
	// NSCpp::Mapvec is short-hand for std::vector<std::map<std::string, std::string>>
	NSCpp::Mapvec vectorOfMaps = response2.respMapVec;
	for(auto censusScale : vectorOfMaps){
		cout << "Score: " << censusScale["SCORE"] << "\nWorld rank: " << censusScale["RANK"] << "\nRegion rank: " << censusScale["RRANK"] <<"\n\n";
	}
	
	// Request happenings shard from world
	NSCpp::Shard response3 = api.APIRequest("world", "happenings");

	// Same than above
	NSCpp::Mapvec events = response3.respMap;

	// Iterate over the vector and print data
	for(auto currentEvent : events){
		cout << "Text: " << currentEvent["TEXT"] << "\nTimestamp: " << currentEvent["TIMESTAMP"] << "\n\n";
	}

	// Request banners shard from a nation named testnation2
	NSCpp::Shard response4 = api.APIRequest("nation", "banners", "testnation2")
	
	// Some shards like banners return multiple, unnamed entries of data, in that case, a vector of strings will be returned through the respVec field
	// NSCpp::Strvec is short-hand for std::vector<std::string>
	NSCpp::Strvec vectorOfStrings = response4.respVec;

	// Iterate over the vector and print data
	for(auto i : vectorOfStrings){
		cout << i << "\n";
	}

	// Request a private shard (dossier) from a nation named okiranoutofnames
	NSCpp::Shard response4 = api.APIRequest("nation", "dossier", "okiranoutofnames", "myultrasafepassword1234");
	
	// Iterate over the vector and print data
	for(auto nation : response4.respVec){
		cout << "Nation: " << nation << "\n";
	}
	return 0;
}
