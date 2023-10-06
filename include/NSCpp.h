#pragma once
#include "curl/curl.h"
#include "definitions.h"
#include "tinyxml2.cpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>

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
	typedef struct { int status; std::string link; } DispatchInfo;
	typedef struct { std::string nation; std::string password; } AuthCredentials;
	class API {
	private:
		std::string _ua;

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

		std::string _httpget(std::string url, bool control_ratelimit = true) {
			CURL* curl;
			CURLcode response;
			std::string respContent;
			std::string requestUserAgent = "User-Agent: " + this->_ua;
			curl_slist* requestHeader = curl_slist_append(NULL, requestUserAgent.c_str()); // Append the User-Agent header to the linked list (libcurl only allows linked lists to be passed as request headers)
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			if (!curl) {
				throw_err("Error: Couldn't initialize CURL.");
			}
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // What URL to request
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow libcurl to follow redirections
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // Disable SSL cert checking (Probably shouldn't do this but idk)
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, requestHeader); // Add request headers (User-Agent)
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, this->_writeResponse); // Parse request body using this function
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respContent); // Write parsed request body into this string variable
			response = curl_easy_perform(curl); // Send request
			if (response != CURLE_OK) {
				throw_err(curl_easy_strerror(response));
			}

			if (!control_ratelimit) {
				curl_easy_cleanup(curl);
				curl_global_cleanup();
				return respContent;
			}

			this->_wait_for_ratelimit(curl);

			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return respContent;
		}

		std::string _buildAPIRequestURI(std::string type, std::string shard, std::string target) {
			std::string uri;
			if (type == "world") {
				uri = "?q=" + shard;
			}
			else {
				uri = "?" + type + "=" + target + "&q=" + shard;
			}

			return uri;
		}

		std::string _parseXML(std::string response, std::string type, std::string shard) {
			tinyxml2::XMLDocument document;
			document.Parse(response.c_str());
			const char* data = document.FirstChildElement(this->_upper(type).c_str())->FirstChildElement(this->_upper(shard).c_str())->GetText();
			return data;
		}

	public:
		API() { ; } // Empty object as result of default constructor :D

		// Doing std::cout << APIObject displays user agent
		friend std::ostream& operator<<(std::ostream& out, API obj) {
			out << obj._ua;
			return out;
		}

		API(std::string scriptFunction, std::string scriptAuthor, std::string scriptUser) {
			if (scriptFunction.empty() || scriptAuthor.empty() || scriptUser.empty()) {
				throw_err("Script function, author and user must be provided.");
			}
			this->_ua = scriptFunction + ", developed by nation=" + scriptAuthor + " and in use by nation=" + scriptUser + ", request sent using NSCpp API wrapper written by nation=jyezet.";
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

		Shard APIRequest(std::string type, std::string shard, std::string target = "") {
			if (type != "world" && type != "region" && type != "nation") {
				throw_err("Request type must be world, region or nation (WA not supported yet).");
			}

			if (type != "world" && target.empty()) {
				throw_err("If request type is not world, target must be provided.");
			}

			if (type.empty()) {
				throw_err("Shard must be provided.");
			}

			std::string uri = this->_buildAPIRequestURI(type, shard, target);
			std::string url = "https://www.nationstates.net/cgi-bin/api.cgi" + uri;
			std::string response = this->_httpget(url);

			Shard data;
			data.shard = shard;
			data.target = target;
			data.response = this->_parseXML(response, type, shard);

			return data;
		}

		DispatchInfo APIDispatch(AuthCredentials credentials, DispatchCategory category, DispatchSubcategory subcategory, int dispatchID = 0, std::string title = "", std::string text = "", std::string action = "add") {
			if (action != DispatchAction::ADD && action != DispatchAction::EDIT && action != DispatchAction::REMOVE) {
				throw_err("Dispatch action must be add, edit or remove.");
			}			
		}
	};
}
