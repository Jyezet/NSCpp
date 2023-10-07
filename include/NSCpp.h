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

namespace errorManagement {
	void _throw_err(std::string error, std::string file, int line) {
		std::cerr << "\nError at file: " << file << ", line: " << line << ", " << error;
		exit(-1);
	}
}

#ifndef throw_err
#define throw_err(error) errorManagement::_throw_err(error, __FILE__, __LINE__)
#endif

namespace NSCpp {
	typedef struct { std::string target; std::string shard; std::string response; } Shard;
	typedef struct { std::string nation; std::string password; } AuthCredentials;
	typedef struct { AuthCredentials credentials; DispatchCategory category; DispatchSubcategory subcategory; std::string title; std::string text; } DispatchInfo;
	typedef std::vector<std::string> strvec;
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

		void _wait_for_ratelimit(CURL* curl) {
			curl_header* ratelimit_remaining_header;
			curl_header* ratelimit_reset_header;
			int ratelimit_remaining;
			int ratelimit_reset;

			// IMPORTANT NOTE FOR THE FUTURE:
			// For some reason only the Gods of C++ understand, 
			// curl_easy_header overwrites all header instances with the last retrieved value
			// That's why I save the value before it's lost

			// Get header info
			curl_easy_header(curl, "Ratelimit-Remaining", 0, CURLH_HEADER, -1, &ratelimit_remaining_header);
			ratelimit_remaining = atoi(ratelimit_remaining_header->value);
			curl_easy_header(curl, "Ratelimit-Reset", 0, CURLH_HEADER, -1, &ratelimit_reset_header);
			ratelimit_reset = atoi(ratelimit_reset_header->value);

			// Ratelimit control
			float wait_time_seconds = (float)ratelimit_reset / ratelimit_remaining;
			std::this_thread::sleep_for(std::chrono::milliseconds((int)wait_time_seconds * 1000));
		}

		std::string _httpget(std::string url, strvec paramNames, strvec paramValues, curl_slist* headers, bool control_ratelimit = true) {
			CURL* curl;
			CURLcode response;
			std::string respContent;
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			if (!curl) {
				throw_err("Error: Couldn't initialize CURL.");
			}

			if (paramNames.size() != paramValues.size()) {
				throw_err("Error: Mismatched URL parameter sizes.");
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

			if (control_ratelimit) {
				this->_wait_for_ratelimit(curl);
			}

			curl_header* xpin_header;
			if (curl_easy_header(curl, "X-Pin", 0, CURLH_HEADER, -1, &xpin_header) == CURLHE_OK) {
				this->_xpin = xpin_header->value;
			}

			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return respContent;
		}

		void _APICommand(AuthCredentials credentials, std::string command, strvec dataNames, strvec dataValues) {
			std::string requestUserAgent = "User-Agent: " + this->_ua;
			std::string requestXPassword = "X-Password: " + credentials.password;
			curl_slist* headersPrepare = curl_slist_append(NULL, requestUserAgent.c_str()); // Append the User-Agent header to the linked list (libcurl only allows linked lists to be passed as request headers)
			headersPrepare = curl_slist_append(headersPrepare, requestXPassword.c_str());
			std::string url = "https://www.nationstates.net/cgi-bin/api.cgi";

			strvec paramNames = {"?nation=", "&c=", "&mode="};
			strvec paramValues = { credentials.nation, command, "prepare"};

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

		std::vector<strvec> _buildAPIRequestURI(std::string type, std::string shard, std::string target) {
			std::vector<strvec> returnData;
			strvec paramNames;
			strvec paramValues;

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
				throw_err("Error: Retrieved a bad URI.");
			}

			return returnData;
		}

		std::string _parseXML(std::string response, std::string type, std::string shard) {
			tinyxml2::XMLDocument document;
			document.Parse(response.c_str());
			const char* data = document.FirstChildElement(this->_upper(type).c_str())->FirstChildElement(this->_upper(shard).c_str())->GetText();
			return data;
		}

	public:
		API() { ; } // Empty object as result of default constructor :D


		API(std::string scriptFunction, std::string scriptAuthor, std::string scriptUser) {
			if (scriptFunction.empty() || scriptAuthor.empty() || scriptUser.empty()) {
				throw_err("Error: Script function, author and user must be provided.");
			}
			this->_ua = scriptFunction + ", developed by nation=" + scriptAuthor + " and in use by nation=" + scriptUser + ", request sent using NSCpp API wrapper written by nation=jyezet.";
		}

		// Copy constructor
		API(API* api) {
			if (api->getUserAgent().empty()) {
				throw_err("Error: Cannot copy API object with empty user agent.");
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
				throw_err("Error: Script function, author and user must be provided.");
			}
			this->_ua = scriptFunction + ", developed by nation=" + scriptAuthor + " and in use by nation=" + scriptUser + ", request sent using NSCpp API wrapper written by nation=jyezet.";
		}

		std::string getUserAgent() {
			return this->_ua;
		}

		void setCustomUserAgent(std::string useragent) {
			if (useragent.empty()) {
				throw_err("Error: User agent must be provided.");
			}
			this->_ua = useragent;
		}

		Shard APIRequest(std::string type, std::string shard, std::string target = "") {
			if (type != "world" && type != "region" && type != "nation") {
				throw_err("Error: Request type must be world, region or nation (WA not supported yet).");
			}

			if (type != "world" && target.empty()) {
				throw_err("Error: If request type is not world, target must be provided.");
			}

			if (type.empty()) {
				throw_err("Error: Shard must be provided.");
			}

			std::vector<strvec> uri = this->_buildAPIRequestURI(type, shard, target);
			std::string url = "https://www.nationstates.net/cgi-bin/api.cgi";
			std::string requestUserAgent = "User-Agent: " + this->_ua;
			curl_slist* headers = curl_slist_append(NULL, requestUserAgent.c_str()); // Append the User-Agent header to the linked list (libcurl only allows linked lists to be passed as request headers)
			std::string response = this->_httpget(url, uri[0], uri[1], headers);

			Shard data;
			data.shard = shard;
			data.target = target;
			data.response = this->_parseXML(response, type, shard);

			return data;
		}

		void APIDispatch(DispatchInfo info, std::string action, int dispatchID = 0) {
			strvec dataNames;
			strvec dataValues;

			if (action != "add" && action != "edit" && action != "remove") {
				throw_err("Error: Dispatch action must be add, edit or remove.");
			}

			if (info.credentials.nation.empty() || info.credentials.password.empty()) {
				throw_err("Error: Please provide an AuthCredentials struct with non-empty fields.");
			}

			if (action != "remove" && (info.title.empty() || info.text.empty() || (int) info.category == 0 || (int) info.subcategory == 0)) {
				throw_err("Error: Dispatch information must be provided if action is not remove.");
			}

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
			this->_APICommand(info.credentials, "dispatch", dataNames, dataValues);
		}
	};
}
