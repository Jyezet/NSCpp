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

	void _throw_err(std::string error, std::string file, int line) {
		std::cerr << "\nError at file: " << file << ", line: " << line << ", " << error;
		exit(-1);
	}

	void _throw_warn(std::string warning, std::string file, int line) {
		if (disableWarnings) {
			return;
		}
		std::cerr << "\Warning at file: " << file << ", line: " << line << ", " << warning;
	}
}

#ifndef throw_err
#define throw_err(error) errorManagement::_throw_err(error, __FILE__, __LINE__)
#endif

#ifndef throw_warn
#define throw_warn(warning) errorManagement::_throw_warn(warning, __FILE__, __LINE__)
#endif

namespace NSCpp {
	typedef std::vector<std::string> Strvec;
	typedef std::vector<std::map<std::string, std::string>> Mapvec;
	typedef std::map<std::string, std::string> Strmap;
	typedef struct { std::string target; std::string shard; std::string response; Mapvec respMapVec; Strvec respVec; Strmap respMap; } Shard;
	typedef struct { std::string nation; std::string password; } AuthCredentials;
	typedef struct { AuthCredentials credentials; DispatchCategory category; DispatchSubcategory subcategory; std::string title; std::string text; } DispatchInfo;
	typedef struct { std::string response; Mapvec respMapVec; Strvec respVec; std::map<std::string, std::string> respMap; } parsedXML;
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
			std::this_thread::sleep_for(std::chrono::milliseconds((int)waitTimeSeconds * 1000));
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
				url += paramNames[i] + curl_easy_escape(curl, paramValues[i].c_str(), 0);
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

			if (shard == "HAPPENINGS") {
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
					tempMap["TYPE"] = "undefined";
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

			if (shard == "BANNERS" || shard == "ADMIRABLES") {
				Strvec strvec;
				std::string elementName = shard.substr(0, shard.size() - 1);
				for (tinyxml2::XMLElement* banner = root->FirstChildElement(elementName.c_str()); banner != NULL; banner = banner->NextSiblingElement(elementName.c_str())) {
					strvec.push_back(banner->GetText());
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
				for (tinyxml2::XMLElement* nation = root->FirstChildElement("NATION"); nation != NULL; nation = nation->NextSiblingElement("NATION")) {
					strvec.push_back(nation->GetText());
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

		Shard APIRequest(std::string type, std::string shard, std::string target = "", std::string password = "", Strvec extraInfoNames = {}, Strvec extraInfoValues = {}) {
			std::string upperShard = this->_upper(shard);
			std::string upperType = this->_upper(type);

			if (type != "world" && type != "region" && type != "nation") {
				throw_err("Request type must be world, region or nation (WA not supported yet).");
			}

			if (type != "world" && target.empty()) {
				throw_err("If request type is not world, target must be provided.");
			}

			if (type.empty()) {
				throw_err("Type must be provided.");
			}

			if (std::count(extraInfoNames.begin(), extraInfoNames.end(), "from") && upperShard != "NOTICES") {
				throw_warn("The 'from' attribute is unnecessary unless the shard is 'notices'.");
			}

			if (type == "nation" && std::distance(validNationShards, std::find(std::begin(validNationShards), std::end(validNationShards), upperShard)) == sizeof(validNationShards) / sizeof(*validNationShards)) {
				throw_err("" + shard + " is not a valid shard for type nation.");
			}

			bool isPrivateShard = std::distance(privateShards, std::find(std::begin(privateShards), std::end(privateShards), upperShard)) != sizeof(privateShards) / sizeof(*privateShards);

			if (type == "nation" && isPrivateShard && password.empty()) {
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
				uri[0].push_back(name);
			}

			for (auto value : extraInfoValues) {
				uri[1].push_back(value);
			}

			std::string response = this->_httpget(url, uri[0], uri[1], headers);

			Shard data;
			data.shard = shard;
			data.target = target;

			if (upperShard == "HAPPENINGS" || upperShard == "DISPATCHLIST" || upperShard == "WABADGES" || upperShard == "ISSUES" || upperShard == "ISSUESUMMARY") {
				data.respMapVec = this->_parseXML(response, upperType, upperShard).respMapVec;
				return data;
			}

			if (upperShard == "BANNERS" || upperShard == "ADMIRABLES" || upperShard == "DOSSIER") {
				data.respVec = this->_parseXML(response, upperType, upperShard).respVec;
				return data;
			}

			if (upperShard == "DEATHS" || upperShard == "ZOMBIE" || upperShard == "FREEDOM" || upperShard == "GOVT") {
				data.respMap = this->_parseXML(response, upperType, upperShard).respMap;
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

			text += "\n\n[i]This was an automated message.[/i]"; // Add small disclaimer at the bottom of the RMB post

			Strvec dataNames = { "region", "text", "v" };
			Strvec dataValues = { region, text, "12" };
			this->_APICommand(credentials, "rmbpost", dataNames, dataValues);
		}
	};
}