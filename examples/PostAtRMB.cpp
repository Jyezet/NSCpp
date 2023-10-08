// If you find an error in this example, contact me through discord (Username: jyezet)
#include <NSCpp.h>
#include <iostream>

using namespace std;

int main() {
  // Create API object, initialize it with an useragent
	NSCpp::API api("Explanation of what your script does", "Nation belonging to the script's author", "Nation belonging to the script's user");

  // Create struct with authentication credentials (With nation and password in it)
  NSCpp::AuthCredentials loginCredentials = {"nation_to_post_with", "ILovePineappleOnPizza*@2"};

  // Pass the APIRMB function your credentials, the text to post and the region whose RMB to post at
  NSCpp::APIRMB(
    loginCredentials,
    "I've just eaten a pizza with pineapple on it and it was delightful.\nAnyways, did y'all ever think that we may be fake people invented for the purpose of a code example? how crazy, right?",
    "nations_region_or_embassyregion_with_rmb_permissions_on"
  );
  
  return 0;
}
