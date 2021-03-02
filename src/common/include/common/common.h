#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <crc32c/crc32c.h>
#include <curl/curl.h>
#include "base64.h"

size_t writeString(void* ptr, size_t size, size_t nmemb, std::string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

size_t writeFile(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

nlohmann::json callJsonAPI(std::string url) {
	auto curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "PD2Launcher/1.1.0");
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	//headers = curl_slist_append(headers, "Authorization: Bearer ");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	std::string response_string;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

	long response_code;
	double elapsed;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);

	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl = NULL;

	// parse and return the response
	return nlohmann::json::parse(response_string);
}

bool hasEnding(std::string const& fullString, std::string const& ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

bool compareCRC(std::filesystem::path filepath, std::string hash) {
	// setup the stream for the file
	std::ifstream ifs(filepath, std::ifstream::binary);
	if (!ifs) {
		return false;
	}

	// read in the file
	std::string file_string = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());

	std::uint32_t result = crc32c::Crc32c(file_string);
	char results_char[4] = { 0 };
	results_char[0] = result >> 24;
	results_char[1] = result >> 16;
	results_char[2] = result >> 8;
	results_char[3] = result;
	std::string result_string = base64_encode((const unsigned char*)results_char, 4);

	if (result_string.compare(hash) == 0) {
		return true;
	}

	return false;
}

void downloadFile(std::string url, std::string filepath) {
	CURL* curl = curl_easy_init();
	if (curl) {
		FILE* fp = fopen(filepath.c_str(), "wb");

		//struct curl_slist* headers = NULL;
		//headers = curl_slist_append(headers, "Content-Type: application/json");
		//headers = curl_slist_append(headers, "Authorization: Bearer ");
		//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFile);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
}