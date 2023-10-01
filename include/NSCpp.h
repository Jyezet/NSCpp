#pragma once
#include <curl/curl.h>
#include <tinyxml2.cpp>
#include <iostream>
#include <algorithm>

namespace errorManagement {
	void _throw_err(std::string error, std::string file, int line) {
		std::cerr << "\nError at file: " << file << ", line: " << line << ", " << error;
		exit(-1);
	}
}

#ifndef throw_err
#define throw_err(error) errorManagement::_throw_err(error, __FILE__, __LINE__)
#endif


namespace APIType {
	std::string WORLD = "world", NATION = "nation", REGION = "region";
}

namespace NSCpp {
	typedef struct { std::string target; std::string shard; std::string response; } Shard;
	class API {
	private:
		std::string _ua;

		static size_t _writeResponse(void* contents, size_t size, size_t nmemb, void* userp) {
			((std::string*)userp)->append((char*) contents, size * nmemb);
			return size * nmemb;
		}

		std::string _request(std::string url) {
			CURL* curl;
			CURLcode response;
			std::string respContent;
			std::string requestUserAgent = "User-Agent: " + this->_ua;
			curl_slist* headers = curl_slist_append(NULL, requestUserAgent.c_str());
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			if (!curl) {
				throw_err("Error: Couldn't initialize CURL.");
			}
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, this->_writeResponse);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respContent);
			response = curl_easy_perform(curl);
			if (response != CURLE_OK) {
				throw_err(curl_easy_strerror(response));
			}
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return respContent;
		}

		std::string _buildAPIRequestURI(std::string type, std::string shard, std::string target) {
			std::string uri;
			if (type == APIType::WORLD) {
				uri = "?q=" + shard;
			}
			else {
				uri = "?" + type + "=" + target + "&q=" + shard;
			}

			return uri;
		}

		std::string _upper(std::string str) {
			transform(str.begin(), str.end(), str.begin(), ::toupper);
			return str;
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
			if (type != APIType::WORLD && type != APIType::REGION && type != APIType::NATION) {
				throw_err("Request type must be world, region or nation (WA not supported yet).");
			}

			if (type != APIType::WORLD && target.empty()) {
				throw_err("If request type is not world, target must be provided.");
			}

			if (type.empty()) {
				throw_err("Shard must be provided.");
			}

			std::string uri = this->_buildAPIRequestURI(type, shard, target);
			std::string url = "https://www.nationstates.net/cgi-bin/api.cgi" + uri;
			std::string response = this->_request(url);

			Shard data;
			data.shard = shard;
			data.target = target;
			data.response = this->_parseXML(response, type, shard);

			return data;
		}
	};
}