#define CURL_STATICLIB
#define _CRT_SECURE_NO_WARNINGS
#include "curl/curl.h"
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include "resources.cpp"
#include <codecvt>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <future>
#include <vector>
#include "crc32c/crc32c.h"
#include "base64.h"

//const char* BUCKET_URL = "https://storage.googleapis.com/storage/v1/b/pd2-launcher/o";
const char* BUCKET_URL = "https://storage.googleapis.com/storage/v1/b/pd2-client-files/o";

namespace fs = std::filesystem;
std::vector<std::future<bool>> pending_futures;

size_t writeString(void* ptr, size_t size, size_t nmemb, std::string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

size_t writeFile(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

nlohmann::json getBucketFiles() {
	auto curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, BUCKET_URL);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "PD2Launcher/1.0.0");
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

bool hasEnding(std::string const& fullString, std::string const& ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

bool compareCRC(fs::path filepath, std::string hash) {
	std::ifstream ifs(filepath, std::ifstream::binary);
	if (!ifs) {
		return false;
	}

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

class frame;
bool _update(frame* window);

class frame : public sciter::window {
public:
	frame() : window(SW_MAIN) {}

	// passport - lists native functions and properties exposed to script:
	SOM_PASSPORT_BEGIN(frame)
		SOM_FUNCS(
			SOM_FUNC(play),
			SOM_FUNC(update)
		)
		SOM_PASSPORT_END

	bool play(sciter::string args) {
		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;
		std::wstring commandLine = L"Diablo II.exe " + args;
		if (CreateProcess(NULL, &commandLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
		{
			//WaitForSingleObject(processInfo.hProcess, INFINITE);
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);

			return true;
		}

		return false;
	}

	bool update() {
		auto fut = std::async(std::launch::async, _update, this);
		pending_futures.push_back(std::move(fut));
		return true;
	}
};

bool _update(frame* window) {
	nlohmann::json json = getBucketFiles();

	// loop through items
	for (auto& element : json["items"]) {
		std::string itemName = element["name"];
		std::string mediaLink = element["mediaLink"];
		std::string crcHash = element["crc32c"];

		// if the itemName ends in a slash, its not a file
		if (hasEnding(itemName, "/")) {
			continue;
		}

		// get the absolute path to the file/item in question
		fs::path path = fs::current_path();
		path = path / itemName;
		path = path.lexically_normal();

		// create any needed directories
		fs::create_directories(path.parent_path());

		// check if it doesnt exist or the crc32c hash doesnt match
		if (!fs::exists(path) || !compareCRC(path, crcHash)) {
			downloadFile(mediaLink, path.string());
		}
	}
	window->call_function("self.finish_update");
	return true;
}


int uimain(std::function<int()> run) {
	// enable debug mode
	//SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);

	// needed for persistant storage
	SciterSetOption(NULL, SCITER_SET_SCRIPT_RUNTIME_FEATURES, ALLOW_FILE_IO | ALLOW_SOCKET_IO | ALLOW_EVAL | ALLOW_SYSINFO);

	sciter::archive::instance().open(aux::elements_of(resources)); // bind resources[] (defined in "resources.cpp") with the archive

	sciter::om::hasset<frame> pwin = new frame();

	// note: this:://app URL is dedicated to the sciter::archive content associated with the application
	pwin->load(WSTR("this://app/main.htm"));

	pwin->expand();

	return run();
}
