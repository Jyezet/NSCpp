#pragma once
#include "curl/curl.h"
#include "definitions.h"
#include "tinyxml2.cpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>
#include <map>

namespace errorManagement {
	bool disableWarnings = false;

	// Errors are critical, and most commonly the fault of the author/user. They cause the closing of the program.
	void _throw_err(std::string error, std::string file, int line) {
		std::cerr << "\nError at file: " << file << ", line: " << line << ", " << error << "\n";
		exit(1);
	}

	// Exceptions are not the fault of the author/user, but of NS API. For stability matters, they don't close the program.
	void _throw_exc(std::string except, std::string file, int line) {
		std::cerr << "\nError at file: " << file << ", line: " << line << ", " << except << "\n";
	}

	// Warnings can be disabled setting the errorManagement::disableWarnings variable to true.
	void _throw_warn(std::string warning, std::string file, int line) {
		if (disableWarnings) {
			return;
		}
		std::cerr << "\nWarning at file: " << file << ", line: " << line << ", " << warning << "\n";
	}
}

#ifndef throw_err
#define throw_err(error) errorManagement::_throw_err(error, __FILE__, __LINE__)
#endif
#ifndef throw_warn
#define throw_warn(warning) errorManagement::_throw_warn(warning, __FILE__, __LINE__)
#endif
#ifndef throw_exc
#define throw_exc(except) errorManagement::_throw_exc(except, __FILE__, __LINE__)
#endif

namespace NSCpp {
	typedef std::vector<std::string> Strvec;
	typedef std::vector<std::map<std::string, std::string>> Mapvec;
	typedef std::map<std::string, std::string> Strmap;
	typedef struct { std::string target; std::string shard; std::string response; Mapvec respMapVec; Strvec respVec; Strmap respMap; Mapvec* optionsPtr; } Shard;
	typedef struct { std::string nation; std::string password; } AuthCredentials;
	typedef struct { AuthCredentials credentials; DispatchCategory category; DispatchSubcategory subcategory; std::string title; std::string text; } DispatchInfo;
	typedef struct { std::string response; Mapvec respMapVec; Strvec respVec; std::map<std::string, std::string> respMap; Mapvec* optionsPtr; } parsedXML;
	
	std::string joinTogether(Strvec strvec) {
		std::string returnData;
		for (auto i : strvec) {
			returnData += i + "+";
		}
		return returnData;
	}
	
	class API {
	private:
		std::string _ua; std::string _xpin = "";

		static size_t _writeResponse(void* contents, size_t size, size_t nmemb, void* userp) {
			((std::string*)userp)->append((char*)contents, size * nmemb);
			return size * nmemb;
		}

		std::string _upper(std::string str) {
			transform(str.begin(), str.end(), str.begin(), ::toupper);
			return str;
		}

		std::string _lower(std::string str) {
			transform(str.begin(), str.end(), str.begin(), ::tolower);
			return str;
		}

		void _waitForRatelimit(CURL* curl) {
			curl_header* ratelimitRemainingHeader;
			curl_header* ratelimitResetHeader;
			int ratelimitRemaining;
			int ratelimitReset;

			// IMPORTANT NOTE FOR THE FUTURE:
			// For some reason only the Gods of C++ understand, 
			// curl_easy_header overwrites all header instances with the last retrieved value
			// That's why I save the value before it's lost

			// Get header info
			curl_easy_header(curl, "Ratelimit-Remaining", 0, CURLH_HEADER, -1, &ratelimitRemainingHeader);
			ratelimitRemaining = atoi(ratelimitRemainingHeader->value);
			curl_easy_header(curl, "Ratelimit-Reset", 0, CURLH_HEADER, -1, &ratelimitResetHeader);
			ratelimitReset = atoi(ratelimitResetHeader->value);

			// Ratelimit control
			float waitTimeSeconds = (float)ratelimitReset / ratelimitRemaining;
			std::this_thread::sleep_for(std::chrono::milliseconds((int) waitTimeSeconds * 1000));
		}

		std::string _httpget(std::string url, Strvec paramNames, Strvec paramValues, curl_slist* headers, bool controlRatelimit = true) {
			CURL* curl;
			CURLcode response;
			std::string respContent;
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			if (!curl) {
				throw_err("Couldn't initialize CURL.");
			}

			if (paramNames.size() != paramValues.size()) {
				throw_err("Mismatched URL parameter sizes.");
			}

			// All this just to escape URL parameters correctly T_T
			for (int i = 0; i < paramNames.size(); i++) {
				std::string currValue = curl_easy_escape(curl, paramValues[i].c_str(), 0);
				if (paramNames[i] == "&q=" || paramNames[i] == "?q=") {
					// Turn all shards to lowercase, as *SOME* API shards are case sensitive (like dossier)
					currValue = this->_lower(currValue);
				}
				url += paramNames[i] + currValue;
			}

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // What URL to request
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow libcurl to follow redirections
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // Disable SSL cert checking (Probably shouldn't do this but idk)
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Add request headers (User-Agent)
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, this->_writeResponse); // Parse request body using this function
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respContent); // Write parsed request body into this string variable
			response = curl_easy_perform(curl); // Send request
			if (response != CURLE_OK) {
				throw_err(curl_easy_strerror(response));
			}

			if (controlRatelimit) {
				this->_waitForRatelimit(curl);
			}

			curl_header* xpinHeader;
			if (curl_easy_header(curl, "X-Pin", 0, CURLH_HEADER, -1, &xpinHeader) == CURLHE_OK) {
				this->_xpin = xpinHeader->value;
			}

			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return respContent;
		}

		void _APICommand(AuthCredentials credentials, std::string command, Strvec dataNames, Strvec dataValues) {
			std::string requestUserAgent = "User-Agent: " + this->_ua;
			std::string requestXPassword = "X-Password: " + credentials.password;
			curl_slist* headersPrepare = curl_slist_append(NULL, requestUserAgent.c_str()); // Append the User-Agent header to the linked list (libcurl only allows linked lists to be passed as request headers)
			headersPrepare = curl_slist_append(headersPrepare, requestXPassword.c_str());
			std::string url = "https://www.nationstates.net/cgi-bin/api.cgi";
			
			Strvec paramNames = { "?nation=", "&c=", "&mode=" };
			Strvec paramValues = { credentials.nation, command, "prepare" };

			for (int i = 0; i < dataNames.size(); i++) {
				paramNames.push_back(dataNames[i]);
				paramValues.push_back(dataValues[i]);
			}

			std::string respPrepare = this->_httpget(url, paramNames, paramValues, headersPrepare);
			if (respPrepare.find("error") != std::string::npos) {
				throw_exc("Nationstates API has thrown an unknown error.");
				return;
			}

			tinyxml2::XMLDocument document;
			document.Parse(respPrepare.c_str());
			const char* rawToken = document.FirstChildElement("NATION")->FirstChildElement("SUCCESS")->GetText();
			std::string token = rawToken;
			std::string requestXPin = "X-Pin: " + this->_xpin;
			curl_slist* headersExecute = curl_slist_append(NULL, requestUserAgent.c_str());
			headersExecute = curl_slist_append(headersExecute, requestXPin.c_str());

			paramValues[2] = "execute";
			paramNames.push_back("&token=");
			paramValues.push_back(token);

			std::string respExecute = this->_httpget(url, paramNames, paramValues, headersExecute);
			if (respExecute.find("error") != std::string::npos) {
				throw_exc("Nationstates API has thrown an unknown error.");
			}
		}

		std::vector<Strvec> _buildAPIRequestURI(std::string type, std::string shard, std::string target) {
			std::vector<Strvec> returnData;
			Strvec paramNames;
			Strvec paramValues;

			if (type == "world") {
				paramNames.push_back("?q=");
				paramValues.push_back(shard);
			}
			else {
				paramNames.push_back("?" + type + "=");
				paramValues.push_back(target);
				paramNames.push_back("&q=");
				paramValues.push_back(shard);
			}

			returnData.push_back(paramNames);
			returnData.push_back(paramValues);

			if (returnData[0].size() != returnData[1].size()) {
				throw_err("Retrieved a bad URI.");
			}

			return returnData;
		}

		parsedXML _parseXML(std::string response, std::string type, std::string shard) {
			tinyxml2::XMLDocument document;
			document.Parse(response.c_str());
			parsedXML resp;
			tinyxml2::XMLElement* root = document.FirstChildElement(type.c_str())->FirstChildElement(shard.c_str());

			if (shard == "HAPPENINGS" || shard == "HISTORY") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* _event = root->FirstChildElement("EVENT"); _event != NULL; _event = _event->NextSiblingElement("EVENT")) {
					std::map<std::string, std::string> tempMap;
					tempMap["TIMESTAMP"] = _event->FirstChildElement("TIMESTAMP")->GetText();
					tempMap["TEXT"] = _event->FirstChildElement("TEXT")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "CENSUSRANKS") {
				Mapvec mapvec;
				tinyxml2::XMLElement* node = root->FirstChildElement("NATIONS");
				for (tinyxml2::XMLElement* nation = node->FirstChildElement("NATION"); nation != NULL; nation = nation->NextSiblingElement("NATION")) {
					std::map<std::string, std::string> tempMap;
					tempMap["NAME"] = nation->FirstChildElement("NAME")->GetText();
					tempMap["RANK"] = nation->FirstChildElement("RANK")->GetText();
					tempMap["SCORE"] = nation->FirstChildElement("SCORE")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "OFFICERS") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* officer = root->FirstChildElement("OFFICER"); officer != NULL; officer = officer->NextSiblingElement("OFFICER")) {
					std::map<std::string, std::string> tempMap;
					tempMap["NATION"] = officer->FirstChildElement("NATION")->GetText();
					tempMap["OFFICE"] = officer->FirstChildElement("OFFICE")->GetText();
					tempMap["AUTHORITY"] = officer->FirstChildElement("AUTHORITY")->GetText();
					tempMap["TIME"] = officer->FirstChildElement("TIME")->GetText();
					tempMap["BY"] = officer->FirstChildElement("BY")->GetText();
					tempMap["ORDER"] = officer->FirstChildElement("ORDER")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "MESSAGES") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* post = root->FirstChildElement("POST"); post != NULL; post = post->NextSiblingElement("POST")) {
					std::map<std::string, std::string> tempMap;
					tempMap["ID"] = post->Attribute("id");
					tempMap["TIMESTAMP"] = post->FirstChildElement("TIMESTAMP")->GetText();
					tempMap["NATION"] = post->FirstChildElement("NATION")->GetText();
					tempMap["STATUS"] = post->FirstChildElement("STATUS")->GetText();
					tempMap["LIKES"] = post->FirstChildElement("LIKES")->GetText();
					tempMap["MESSAGE"] = post->FirstChildElement("MESSAGE")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "POLICIES") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* policy = root->FirstChildElement("POLICY"); policy != NULL; policy = policy->NextSiblingElement("POLICY")) {
					std::map<std::string, std::string> tempMap;
					tempMap["NAME"] = policy->FirstChildElement("NAME")->GetText();
					tempMap["PIC"] = policy->FirstChildElement("PIC")->GetText();
					tempMap["CAT"] = policy->FirstChildElement("CAT")->GetText();
					tempMap["DESC"] = policy->FirstChildElement("DESC")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "UNREAD") {
				Strmap strmap;
				strmap["ISSUES"] = root->FirstChildElement("ISSUES")->GetText();
				strmap["TELEGRAMS"] = root->FirstChildElement("TELEGRAMS")->GetText();
				strmap["NOTICES"] = root->FirstChildElement("NOTICES")->GetText();
				strmap["RMB"] = root->FirstChildElement("RMB")->GetText();
				strmap["WA"] = root->FirstChildElement("WA")->GetText();
				strmap["NEWS"] = root->FirstChildElement("NEWS")->GetText();
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "POLL") {
				Strmap strmap;
				Mapvec* options = new Mapvec();
				strmap["ID"] = root->Attribute("id");
				strmap["TITLE"] = root->FirstChildElement("TITLE")->GetText();
				strmap["TEXT"] = root->FirstChildElement("TEXT")->GetText();
				strmap["REGION"] = root->FirstChildElement("REGION")->GetText();
				strmap["START"] = root->FirstChildElement("START")->GetText();
				strmap["STOP"] = root->FirstChildElement("STOP")->GetText();
				strmap["AUTHOR"] = root->FirstChildElement("AUTHOR")->GetText();
				tinyxml2::XMLElement* optionsNode = root->FirstChildElement("OPTIONS");
				for (tinyxml2::XMLElement* option = optionsNode->FirstChildElement("OPTION"); option != NULL; option = option->NextSiblingElement("OPTION")) {
					Strmap tempMap;
					tempMap["ID"] = option->Attribute("id");
					tempMap["OPTIONTEXT"] = option->FirstChildElement("OPTIONTEXT")->GetText();
					tempMap["VOTES"] = option->FirstChildElement("VOTES")->GetText();
					if (option->FirstChildElement("VOTERS") != NULL && option->FirstChildElement("VOTERS")->GetText() != NULL) tempMap["VOTERS"] = option->FirstChildElement("VOTERS")->GetText();
					options->push_back(tempMap);
				}
				resp.respMap = strmap;
				resp.optionsPtr = options;
				return resp;
			}

			if (shard == "GAVOTE") {
				Strmap strmap;
				strmap["FOR"] = root->FirstChildElement("FOR")->GetText();
				strmap["AGAINST"] = root->FirstChildElement("AGAINST")->GetText();
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "SECTORS") {
				Strmap strmap;
				strmap["BLACKMARKET"] = root->FirstChildElement("BLACKMARKET")->GetText();
				strmap["GOVERNMENT"] = root->FirstChildElement("GOVERNMENT")->GetText();
				strmap["INDUSTRY"] = root->FirstChildElement("INDUSTRY")->GetText();
				strmap["PUBLIC"] = root->FirstChildElement("PUBLIC")->GetText();
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "CENSUS") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* scale = root->FirstChildElement("SCALE"); scale != NULL; scale = scale->NextSiblingElement("SCALE")) {
					std::map<std::string, std::string> tempMap;
					tempMap["ID"] = scale->Attribute("id");
					tempMap["SCORE"] = scale->FirstChildElement("SCORE")->GetText();
					tempMap["RANK"] = scale->FirstChildElement("RANK")->GetText();
					if(type == "NATION") tempMap["RRANK"] = scale->FirstChildElement("RRANK")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "NOTICES") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* notice = root->FirstChildElement("NOTICE"); notice != NULL; notice = notice->NextSiblingElement("NOTICE")) {
					std::map<std::string, std::string> tempMap;

					// This absolute mess of a code is the result of NS API deciding to omit certain entries on some response bits but not on others. To avoid exceptions, I check if the pointer to that entry is not NULL.
					// But it gets worse, if a node exists but it's empty, the XML parser will not return a null pointer to it but the string it contains will be null, that's why I'm running a double check on every statement
					// Now you know who to thank for these beautiful features /s
					if (notice->FirstChildElement("NEW")       != NULL && notice->FirstChildElement("NEW")->GetText()       != NULL)       tempMap["NEW"]       = notice->FirstChildElement("NEW")->GetText();
					if (notice->FirstChildElement("OK")        != NULL && notice->FirstChildElement("OK")->GetText()        != NULL)       tempMap["OK"]        = notice->FirstChildElement("OK")->GetText();
					if (notice->FirstChildElement("TEXT")      != NULL && notice->FirstChildElement("TEXT")->GetText()      != NULL)       tempMap["TEXT"]      = notice->FirstChildElement("TEXT")->GetText();
					if (notice->FirstChildElement("TIMESTAMP") != NULL && notice->FirstChildElement("TIMESTAMP")->GetText() != NULL)       tempMap["TIMESTAMP"] = notice->FirstChildElement("TIMESTAMP")->GetText();
					if (notice->FirstChildElement("TITLE")     != NULL && notice->FirstChildElement("TITLE")->GetText()     != NULL)       tempMap["TITLE"]     = notice->FirstChildElement("TITLE")->GetText();
					if (notice->FirstChildElement("TYPE")      != NULL && notice->FirstChildElement("TYPE")->GetText()      != NULL)       tempMap["TYPE"]      = notice->FirstChildElement("TYPE")->GetText();
					if (notice->FirstChildElement("TYPE_ICON") != NULL && notice->FirstChildElement("TYPE_ICON")->GetText() != NULL)       tempMap["TYPE_ICON"] = notice->FirstChildElement("TYPE_ICON")->GetText();
					if (notice->FirstChildElement("URL")       != NULL && notice->FirstChildElement("URL")->GetText()       != NULL)       tempMap["URL"]       = notice->FirstChildElement("URL")->GetText();
					if (notice->FirstChildElement("WHO")       != NULL && notice->FirstChildElement("WHO")->GetText()       != NULL)       tempMap["WHO"]       = notice->FirstChildElement("WHO")->GetText();
					if (notice->FirstChildElement("WHO_URL")   != NULL && notice->FirstChildElement("WHO_URL")->GetText()   != NULL)       tempMap["WHO_URL"]   = notice->FirstChildElement("WHO_URL")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "ISSUESUMMARY") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* issue = root->FirstChildElement("ISSUE"); issue != NULL; issue = issue->NextSiblingElement("ISSUE")) {
					std::map<std::string, std::string> tempMap;
					tempMap["ID"] = issue->Attribute("id");
					tempMap["TITLE"] = issue->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "WABADGES") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* badge = root->FirstChildElement("WABADGE"); badge != NULL; badge = badge->NextSiblingElement("WABADGE")) {
					std::map<std::string, std::string> tempMap;
					tempMap["TYPE"] = badge->Attribute("type");
					tempMap["RESOLUTION"] = badge->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "ISSUES") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* issue = root->FirstChildElement("ISSUE"); issue != NULL; issue = issue->NextSiblingElement("ISSUE")) {
					std::map<std::string, std::string> tempMap;
					tempMap["TITLE"] = issue->FirstChildElement("TITLE")->GetText();
					tempMap["ID"] = issue->Attribute("id");
					tempMap["TEXT"] = issue->FirstChildElement("TEXT")->GetText();
					tempMap["AUTHOR"] = issue->FirstChildElement("AUTHOR")->GetText();
					tempMap["EDITOR"] = issue->FirstChildElement("EDITOR")->GetText();
					tempMap["PIC1"] = issue->FirstChildElement("PIC1")->GetText();
					tempMap["PIC2"] = issue->FirstChildElement("PIC2")->GetText();
					int issueCount = 0;
					for (tinyxml2::XMLElement* option = issue->FirstChildElement("OPTION"); option != NULL; option = option->NextSiblingElement("OPTION")) {
						tempMap["OPTION " + std::to_string(issueCount)] = option->GetText();
						issueCount++;
					}
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "DISPATCHLIST") {
				Mapvec mapvec;
				for (tinyxml2::XMLElement* dispatch = root->FirstChildElement("DISPATCH"); dispatch != NULL; dispatch = dispatch->NextSiblingElement("DISPATCH")) {
					std::map<std::string, std::string> tempMap;
					tempMap["TITLE"] = dispatch->FirstChildElement("TITLE")->GetText();
					tempMap["ID"] = dispatch->Attribute("id");
					tempMap["AUTHOR"] = dispatch->FirstChildElement("AUTHOR")->GetText();
					tempMap["CATEGORY"] = dispatch->FirstChildElement("CATEGORY")->GetText();
					tempMap["SUBCATEGORY"] = dispatch->FirstChildElement("SUBCATEGORY")->GetText();
					tempMap["CREATED"] = dispatch->FirstChildElement("CREATED")->GetText();
					tempMap["EDITED"] = dispatch->FirstChildElement("EDITED")->GetText();
					tempMap["VIEWS"] = dispatch->FirstChildElement("VIEWS")->GetText();
					tempMap["SCORE"] = dispatch->FirstChildElement("SCORE")->GetText();
					mapvec.push_back(tempMap);
				}
				resp.respMapVec = mapvec;
				return resp;
			}

			if (shard == "BANNERS" || shard == "ADMIRABLES" || shard == "NOTABLES") {
				Strvec strvec;
				std::string elementName = shard.substr(0, shard.size() - 1);
				for (tinyxml2::XMLElement* node = root->FirstChildElement(elementName.c_str()); node != NULL; node = node->NextSiblingElement(elementName.c_str())) {
					strvec.push_back(node->GetText());
				}
				resp.respVec = strvec;
				return resp;
			}

			if (shard == "EMBASSIES") {
				Strvec strvec;
				for (tinyxml2::XMLElement* embassy = root->FirstChildElement("EMBASSY"); embassy != NULL; embassy = embassy->NextSiblingElement("EMBASSY")) {
					strvec.push_back(embassy->GetText());
				}
				resp.respVec = strvec;
				return resp;
			}

			if (shard == "LEGISLATION") {
				Strvec strvec;
				for (tinyxml2::XMLElement* law = root->FirstChildElement("LAW"); law != NULL; law = law->NextSiblingElement("LAW")) {
					strvec.push_back(law->GetText());
				}
				resp.respVec = strvec;
				return resp;
			}

			// There are instances where a shard's request name is not the same as the shard's XML name (Thank you NS devs)
			if (shard.find("CUSTOM") != std::string::npos) {
				std::string fixedShard = shard.substr(6, shard.size());
				const char* data = document.FirstChildElement(type.c_str())->FirstChildElement(fixedShard.c_str())->GetText();
				resp.response = data;
				return resp;
			}

			// Same than above
			if (shard == "WA") {
				const char* data = document.FirstChildElement(type.c_str())->FirstChildElement("UNSTATUS")->GetText();
				resp.response = data;
				return resp;
			}

			// And above
			if (shard == "BANLIST" && type == "REGION") {
				const char* data = document.FirstChildElement(type.c_str())->FirstChildElement("BANNED")->GetText();
				resp.response = data;
				return resp;
			}

			// And above...
			if (shard == "WANATIONS" && type == "REGION") {
				const char* data = document.FirstChildElement(type.c_str())->FirstChildElement("UNNATIONS")->GetText();
				resp.response = data;
				return resp;
			}

			// This is getting a bit repetitive
			if (shard == "NUMWANATIONS" && type == "REGION") {
				const char* data = document.FirstChildElement(type.c_str())->FirstChildElement("NUMUNNATIONS")->GetText();
				resp.response = data;
				return resp;
			}

			if (shard == "ZOMBIE") {
				Strmap strmap;
				strmap["ZACTION"] = root->FirstChildElement("ZACTION")->GetText();
				strmap["ZACTIONINTENDED"] = root->FirstChildElement("ZACTIONINTENDED")->GetText();
				strmap["SURVIVORS"] = root->FirstChildElement("SURVIVORS")->GetText();
				strmap["ZOMBIES"] = root->FirstChildElement("ZOMBIES")->GetText();
				strmap["DEAD"] = root->FirstChildElement("DEAD")->GetText();
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "DEATHS") {
				Strmap strmap;
				for (auto cause = root->FirstChildElement("CAUSE"); cause != NULL; cause = cause->NextSiblingElement("CAUSE")) {
					strmap[cause->Attribute("type")] = cause->GetText();
				}
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "GOVT") {
				Strmap strmap;
				strmap["ADMINISTRATION"] = root->FirstChildElement("ADMINISTRATION")->GetText();
				strmap["DEFENCE"] = root->FirstChildElement("DEFENCE")->GetText();
				strmap["EDUCATION"] = root->FirstChildElement("EDUCATION")->GetText();
				strmap["ENVIRONMENT"] = root->FirstChildElement("ENVIRONMENT")->GetText();
				strmap["HEALTHCARE"] = root->FirstChildElement("HEALTHCARE")->GetText();
				strmap["COMMERCE"] = root->FirstChildElement("COMMERCE")->GetText();
				strmap["INTERNATIONALAID"] = root->FirstChildElement("INTERNATIONALAID")->GetText();
				strmap["LAWANDORDER"] = root->FirstChildElement("LAWANDORDER")->GetText();
				strmap["PUBLICTRANSPORT"] = root->FirstChildElement("PUBLICTRANSPORT")->GetText();
				strmap["SOCIALEQUALITY"] = root->FirstChildElement("SOCIALEQUALITY")->GetText();
				strmap["SPIRITUALITY"] = root->FirstChildElement("SPIRITUALITY")->GetText();
				strmap["WELFARE"] = root->FirstChildElement("WELFARE")->GetText();
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "FREEDOM") {
				Strmap strmap;
				strmap["CIVILRIGHTS"] = root->FirstChildElement("CIVILRIGHTS")->GetText();
				strmap["ECONOMY"] = root->FirstChildElement("ECONOMY")->GetText();
				strmap["POLITICALFREEDOM"] = root->FirstChildElement("POLITICALFREEDOM")->GetText();
				resp.respMap = strmap;
				return resp;
			}

			if (shard == "DOSSIER") {
				Strvec strvec;
				for (auto nation = root->FirstChildElement("NATION"); nation != NULL; nation = nation->NextSiblingElement("NATION")) {
					std::string nationName = nation->GetText();
					strvec.push_back(nationName);
				}
				resp.respVec = strvec;
				return resp;
			}

			if (shard == "RDOSSIER") {
				Strvec strvec;
				for (auto region = root->FirstChildElement("REGION"); region != NULL; region = region->NextSiblingElement("REGION")) {
					strvec.push_back(region->GetText());
				}
				resp.respVec = strvec;
				return resp;
			}

			const char* data = root->GetText();
			resp.response = data;
			return resp;
		}

	public:
		API() { ; } // Empty object as result of default constructor :D


		API(std::string scriptFunction, std::string scriptAuthor, std::string scriptUser) {
			if (scriptFunction.empty() || scriptAuthor.empty() || scriptUser.empty()) {
				throw_err("Script function, author and user must be provided.");
			}

			this->_ua = scriptFunction + ", developed by nation=" + scriptAuthor + " and in use by nation=" + scriptUser + ", request sent using NSCpp API wrapper written by nation=jyezet.";
		}

		// Copy constructor
		API(API* api) {
			if (api->getUserAgent().empty()) {
				throw_err("Cannot copy API object with empty user agent.");
			}

			this->_ua = api->getUserAgent();
			delete api; // There can only be one object at a time
		}

		// Doing std::cout << APIObject displays user agent
		friend std::ostream& operator<<(std::ostream& out, API obj) {
			out << obj._ua;
			return out;
		}

		void setUserAgent(std::string scriptFunction, std::string scriptAuthor, std::string scriptUser) {
			if (scriptFunction.empty() || scriptAuthor.empty() || scriptUser.empty()) {
				throw_err("Script function, author and user must be provided.");
			}

			this->_ua = scriptFunction + ", developed by nation=" + scriptAuthor + " and in use by nation=" + scriptUser + ", request sent using NSCpp API wrapper written by nation=jyezet.";
		}

		std::string getUserAgent() {
			return this->_ua;
		}

		void setCustomUserAgent(std::string useragent) {
			if (useragent.empty()) {
				throw_err("User agent must be provided.");
			}

			this->_ua = useragent;
		}

		Shard APIRequest(std::string type, std::string shard, std::string target = "", Strvec extraInfoNames = {}, Strvec extraInfoValues = {}, std::string password = "") {
			std::string upperShard = this->_upper(shard);
			std::string upperType = this->_upper(type);

			if (upperType != "WORLD" && upperType != "REGION" && upperType != "NATION" && upperType != "WA") {
				throw_err("Request type must be world, region or nation.");
			}

			if (type.empty()) {
				throw_err("Type must be provided.");
			}

			if (upperType != "WORLD" && target.empty()) {
				throw_err("If request type is not world, target must be provided.");
			}

			if (std::count(extraInfoNames.begin(), extraInfoNames.end(), "from") && upperShard != "NOTICES") {
				throw_warn("The 'from' attribute is unnecessary unless the shard is 'notices'.");
			}

			if ((std::count(extraInfoNames.begin(), extraInfoNames.end(), "scale") || std::count(extraInfoNames.begin(), extraInfoNames.end(), "mode")) && upperShard != "CENSUS" && upperShard != "CENSUSRANKS") {
				throw_warn("The 'mode' and 'scale' attributes are unnecessary unless the shard is 'census' or 'censusranks'.");
			}

			if ((std::count(extraInfoNames.begin(), extraInfoNames.end(), "limit") || std::count(extraInfoNames.begin(), extraInfoNames.end(), "offset") || std::count(extraInfoNames.begin(), extraInfoNames.end(), "fromid")) && upperShard != "CENSUS" && upperShard != "MESSAGES") {
				throw_warn("The 'limit', 'offset' and 'fromid' attributes are unnecessary unless the shard is 'messages'.");
			}

			if (upperType == "NATION" && std::distance(validNationShards, std::find(std::begin(validNationShards), std::end(validNationShards), upperShard)) == sizeof(validNationShards) / sizeof(*validNationShards)) {
				throw_err("'" + shard + "' is not a valid shard for type nation.");
			}

			if (upperType == "REGION" && std::distance(validRegionShards, std::find(std::begin(validRegionShards), std::end(validRegionShards), upperShard)) == sizeof(validNationShards) / sizeof(*validNationShards)) {
				throw_err("'" + shard + "' is not a valid shard for type region.");

			}

			if (upperType == "WORLD" && std::distance(validWorldShards, std::find(std::begin(validWorldShards), std::end(validWorldShards), upperShard)) == sizeof(validWorldShards) / sizeof(*validWorldShards)) {
				throw_err("'" + shard + "' is not a valid shard for type world.");
			}

			if (upperType == "WA" && std::distance(validWAShards, std::find(std::begin(validWAShards), std::end(validWAShards), upperShard)) == sizeof(validWAShards) / sizeof(*validWAShards)) {
				throw_err("'" + shard + "' is not a valid shard for type WA.");
			}

			bool isPrivateShard = std::distance(privateShards, std::find(std::begin(privateShards), std::end(privateShards), upperShard)) != sizeof(privateShards) / sizeof(*privateShards);

			if (upperType == "NATION" && isPrivateShard && password.empty()) {
				throw_err("Target's password must be provided to access private shard '" + upperShard + "'.");
			}

			if (!isPrivateShard && !password.empty()) {
				throw_warn("The 'password' argument is unnecessary unless the shard is private.");
			}

			std::vector<Strvec> uri = this->_buildAPIRequestURI(type, upperShard, target);
			std::string url = "https://www.nationstates.net/cgi-bin/api.cgi";
			std::string requestUserAgent = "User-Agent: " + this->_ua;
			curl_slist* headers = curl_slist_append(NULL, requestUserAgent.c_str()); // Append the User-Agent header to the linked list (libcurl only allows linked lists to be passed as request headers)
			
			if (isPrivateShard) {
				std::string XPassword = "X-Password: " + password;
				headers = curl_slist_append(headers, XPassword.c_str());
			}

			for (auto name : extraInfoNames) {
				uri[0].push_back("&" + name + "=");
			}

			for (auto value : extraInfoValues) {
				uri[1].push_back(value);
			}

			std::string response = this->_httpget(url, uri[0], uri[1], headers);
			
			Shard data;
			if (response.find("error") != std::string::npos) {
				throw_exc("Nationstates API has thrown an unknown error.");
				return data;
			}
 			data.shard = shard;
			data.target = target;

			if (upperShard == "HAPPENINGS" || upperShard == "HISTORY" || upperShard == "DISPATCHLIST" || upperShard == "WABADGES" || upperShard == "ISSUES" || upperShard == "ISSUESUMMARY" || upperShard == "NOTICES" || upperShard == "CENSUS" || upperShard == "POLICIES" || upperShard == "OFFICERS" || upperShard == "MESSAGES" || upperShard == "CENSUSRANKS") {
				data.respMapVec = this->_parseXML(response, upperType, upperShard).respMapVec;
				return data;
			}

			if (upperShard == "BANNERS" || upperShard == "ADMIRABLES" || upperShard == "DOSSIER" || upperShard == "RDOSSIER" || upperShard == "NOTABLES" || upperShard == "LEGISLATION" || upperShard == "EMBASSIES") {
				data.respVec = this->_parseXML(response, upperType, upperShard).respVec;
				return data;
			}

			if (upperShard == "DEATHS" || upperShard == "ZOMBIE" || upperShard == "FREEDOM" || upperShard == "GOVT" || upperShard == "UNREAD" || upperShard == "SECTORS" || upperShard == "GAVOTE" || upperShard == "POLL") {
				parsedXML xml = this->_parseXML(response, upperType, upperShard);
				data.respMap = xml.respMap;
				if (upperShard == "POLL") data.optionsPtr = xml.optionsPtr;
				return data;
			}

			data.response = this->_parseXML(response, upperType, upperShard).response;
			return data;
		}

		void APIDispatch(DispatchInfo info, std::string action, int dispatchID = 0) {
			if (action != "add" && action != "edit" && action != "remove") {
				throw_err("Dispatch action must be add, edit or remove.");
			}

			if (info.credentials.nation.empty() || info.credentials.password.empty()) {
				throw_err("Please provide an AuthCredentials struct with non-empty fields.");
			}

			if (action != "remove" && (info.title.empty() || info.text.empty() || (int)info.category == 0 || (int)info.subcategory == 0)) {
				throw_err("Dispatch information must be provided if action is not remove.");
			}

			Strvec dataNames;
			Strvec dataValues;

			dataNames.push_back("&dispatch=");
			dataValues.push_back(action);

			if (action == "add" || action == "edit") {
				dataNames.push_back("&title=");
				dataValues.push_back(info.title);
				dataNames.push_back("&text=");
				dataValues.push_back(info.text);
				dataNames.push_back("&category=");
				dataValues.push_back(std::to_string((int)info.category));
				dataNames.push_back("&subcategory=");
				dataValues.push_back(std::to_string((int)info.subcategory));
			}

			if (action == "edit" || action == "remove") {
				dataNames.push_back("&dispatchid=");
				dataValues.push_back(std::to_string(dispatchID));
			}

			dataNames.push_back("v");
			dataValues.push_back("12");

			this->_APICommand(info.credentials, "dispatch", dataNames, dataValues);
		}

		void APIRMB(AuthCredentials credentials, std::string text, std::string region) {
			if (credentials.nation.empty() || credentials.password.empty()) {
				throw_err("Please provide an AuthCredentials struct with non-empty fields.");
			}

			if (text.empty() || region.empty()) {
				throw_err("Text and region must be provided.");
			}

			Strvec dataNames = { "&region=", "&text=", "&v=" };
			Strvec dataValues = { region, text, "12" };
			this->_APICommand(credentials, "rmbpost", dataNames, dataValues);
		}
	};
}
