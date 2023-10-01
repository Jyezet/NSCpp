#include <NSCpp.h>
#include <iostream>

using namespace std;

int main() {
  // Create API object, initialize it with an useragent
	NSCpp::API api("Explanation of what your script does", "Nation belonging to the script's author", "Nation belonging to the script's user");

  // Request population shard from the nation named testnation, returns a struct containing requested shard, request type and API response
	NSCpp::Shard response = api.APIRequest(APIType::NATION, "population", "testnation");

  // Print the API response
	cout << response.response;
	return 0;
}
