#include <NSCpp.h>
#include <iostream>

using namespace std;

int main() {
  // Create API object, initialize it with an useragent
	NSCpp::API api("Explanation of what your script does", "Nation belonging to the script's author", "Nation belonging to the script's user");

  // Create struct with authentication credentials (With nation and password in it)
  NSCpp::AuthCredentials loginCredentials = {"amazing_nation", "pepperoni123"}; 
  
  // Create struct containing information to be posted in the dispatch
  // Important note: There are 2 pairs of conflicting subcategories, culture and military
  // To tell them apart they'll appear as FactCulture/AccCulture and FactMilitary/AccMilitary.
  NSCpp::DispatchInfo information = {loginCredentials, NSCpp::DispatchCategory::Factbook, NSCpp::DispatchSubcategory::FactCulture, "dispatch title", "Dear reader: today I ate a burrito.\nThanks for reading."};

  // Call the function
  // The second argument is the action to perform (add/edit/remove)
  // The third argument is a dispatch ID (Only passed if you want to edit or remove a dispatch)
  api.APIDispatch(information, "add");

  // Modify the information struct
  information.text = "Dear reader: I've decided to edit this dispatch.\nThank you.";

  // Call the function in edit mode, passing a dispatch ID
  api.APIDispatch(information, "edit", 123456);

  // Delete a dispatch
  // In this case, the information struct only has to include login credentials, as all other fields will be ignored.
  api.APIDispatch(information, "remove", 123456);
  
  return 0;
}
