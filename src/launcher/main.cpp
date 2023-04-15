#define CURL_STATICLIB
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <future>
#include <vector>
#include <nlohmann/json.hpp>
#include <common/common.h>
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include "resources.cpp"
#include "inihelper.h"
#include <aux-cvt.h>
namespace fs = std::filesystem;


std::string LAUNCHER_BUCKET = "https://storage.googleapis.com/storage/v1/b/pd2-launcher-update/o";
std::string CLIENT_FILES_BUCKET = "https://storage.googleapis.com/storage/v1/b/pd2-client-files/o";

std::string BETA_LAUNCHER_BUCKET = "https://storage.googleapis.com/storage/v1/b/pd2-beta-launcher-update/o";
std::string BETA_CLIENT_FILES_BUCKET = "https://storage.googleapis.com/storage/v1/b/pd2-beta-client-files/o";

std::string lastFilterDownload = "";

std::vector<std::string> dont_update = { "D2.LNG", "BnetLog.txt", "ProjectDiablo.cfg", "ddraw.ini", "default.filter", "loot.filter", "UI.ini", "d2gl.yaml" };
HANDLE pd2Mutex;

void updateLauncher() {
	nlohmann::json json = callJsonAPI(LAUNCHER_BUCKET);

	// Update updater
	for (auto& element : json["items"]) {
		std::string itemName = element["name"];
		std::string mediaLink = element["mediaLink"];
		std::string crcHash = element["crc32c"];

		// get the absolute path to the file/item in question
		fs::path path = fs::current_path();
		path = path / itemName;
		path = path.lexically_normal();

		if (itemName == "updater.exe") {
			if (!fs::exists(path)) {
				std::cout << "Downloading: " << itemName << "\n";
				downloadFile(mediaLink, path.string());
			}
			else if (!compareCRC(path, crcHash)) {
				std::cout << "CRC Mismatch: " << itemName << "\n";
				downloadFile(mediaLink, path.string());
			}
		}
	}

	// Update launcher
	for (auto& element : json["items"]) {
		std::string itemName = element["name"];
		std::string mediaLink = element["mediaLink"];
		std::string crcHash = element["crc32c"];

		// get the absolute path to the file/item in question
		fs::path path = fs::current_path();
		path = path / itemName;
		path = path.lexically_normal();

		if (itemName == "PD2Launcher.exe" && !compareCRC(path, crcHash)) {
			MessageBox(NULL, L"An update for the launcher was found and will be downloaded now.", L"Update ready!", MB_OK | MB_ICONINFORMATION);

			// close the mutex
			if (pd2Mutex) {
				ReleaseMutex(pd2Mutex);
				CloseHandle(pd2Mutex);
			}

			// launch the updater
			STARTUPINFO info = { sizeof(info) };
			PROCESS_INFORMATION processInfo;
			std::wstring commandLine = L"updater.exe /f /l";
			if (CreateProcess(NULL, &commandLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) {
				CloseHandle(processInfo.hProcess);
				CloseHandle(processInfo.hThread);

				// exit the launcher
				exit(0);
			}
		}
	}
}

void updateClientFiles() {
	nlohmann::json json = callJsonAPI(CLIENT_FILES_BUCKET);

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
		if (!fs::exists(path)) {
			downloadFile(mediaLink, path.string());
		}
		else if (!compareCRC(path, crcHash)) {
			// Don't update certain files (config files, etc.)
			if (std::find(dont_update.begin(), dont_update.end(), itemName) == dont_update.end()) {
				downloadFile(mediaLink, path.string());
			}
		}
	}
}

std::string ws2s(const std::wstring& wstr) {
	int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
	std::string str(count, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
	return str;
}

bool lootFilter(sciter::string author, sciter::string filter, sciter::string download_url, sciter::string url) {
	fs::path defaultFilterPath = (fs::current_path() / "loot.filter").lexically_normal();
	fs::path path = (fs::current_path() / "filters").lexically_normal();

	if (author == std::wstring("local", "local" + strlen("local"))) {
		// Is local folder
		path = (path / "local").lexically_normal();
		fs::path filterPath = (path / filter).lexically_normal();

		if (!fs::exists(filterPath)) {
			return false;
		}

		if (fs::exists(defaultFilterPath)) {
			fs::remove(defaultFilterPath);
		}

		fs::create_symlink(filterPath, defaultFilterPath);
	}
	else {
		if (ws2s(author) == "" || ws2s(filter) == "" || ws2s(download_url) == "" || ws2s(url) == "" || lastFilterDownload == ws2s(download_url)) {
			return false;
		}

		path = (path / "online").lexically_normal();
		fs::path authorPath = (path / author).lexically_normal();
		fs::path filterPath = (authorPath / filter).lexically_normal();

		if (!fs::exists(authorPath)) {
			fs::create_directories(authorPath);
		}

		downloadFile(ws2s(download_url), filterPath.string());

		if (!fs::exists(filterPath)) {
			return false;
		}

		if (fs::exists(defaultFilterPath)) {
			fs::remove(defaultFilterPath);
		}

		fs::copy(filterPath, defaultFilterPath);

		lastFilterDownload = ws2s(download_url);
	}

	return true;
}

void checkLootFilterFileStructure() {
	fs::path filtersPath = (fs::current_path() / "filters").lexically_normal();
	fs::path localPath = (filtersPath / "local").lexically_normal();
	fs::path onlinePath = (filtersPath / "online").lexically_normal();

	if (!fs::exists(filtersPath)) {
		fs::create_directories(filtersPath);
	}

	if (!fs::exists(localPath)) {
		fs::create_directories(localPath);

		fs::path defaultFilterPath = (fs::current_path() / "default.filter").lexically_normal();
		fs::path lootFilterPath = (fs::current_path() / "loot.filter").lexically_normal();

		if (fs::exists(defaultFilterPath)) {
			fs::copy(defaultFilterPath, localPath / "default.filter");
		}

		if (fs::exists(lootFilterPath)) {
			fs::copy(lootFilterPath, localPath / "loot.filter");
		}
	}

	if (!fs::exists(onlinePath)) {
		fs::create_directories(onlinePath);
	}
}

sciter::value getDdrawIni() {
	//Read ddraw.ini file and return a key/value map as sciter::value
	sciter::value ddrawoptions;

	mINI::INIFile file("ddraw.ini");
	mINI::INIStructure ini;

	file.read(ini);

	for (auto const& it : ini)
	{
		auto const& section = it.first;
		auto const& collection = it.second;
		for (auto const& it2 : collection)
		{
			auto const& key = it2.first;
			auto const& value = it2.second;
			ddrawoptions.set_item(key, value);
		}
	}
	return ddrawoptions;
}

void setDdrawIni(sciter::value ddrawoptions) {
	mINI::INIFile file("ddraw.ini");
	mINI::INIStructure ini;

	file.read(ini);

	if (ini.size() == 0) {
		writeDefaultDdrawIni(file, ini);
		file.write(ini);
		return;
	}

	for (auto const& it : ini)
	{
		auto const& section = it.first;
		auto const& collection = it.second;
		for (auto const& it2 : collection)
		{
			auto const& key = it2.first;
			if (ddrawoptions.get_item(key).is_bool()) {
				bool value = ddrawoptions.get_item(key).get(false);
				ini[section][key] = (value) ? "true" : "false";
			}
			else if (ddrawoptions.get_item(key).is_int()) {
				auto value = aux::itoa(ddrawoptions.get_item(key).get(1));
				ini[section][key] = value;
			}
			else {
				auto value = aux::w2a(ddrawoptions.get_item(key).get(L""));
				ini[section][key] = value;
			}
		}
	}
	file.write(ini);
	return;
}

class frame : public sciter::window {
public:
	frame() : window(SW_MAIN) {}

	// passport - lists native functions and properties exposed to script:
	SOM_PASSPORT_BEGIN(frame)
		SOM_FUNCS(
			SOM_FUNC(play),
			SOM_FUNC(setLootFilter),
			SOM_FUNC(getLocalFiles),
			SOM_FUNC(getDdrawOptions),
			SOM_FUNC(setDdrawOptions)
		)
		SOM_PASSPORT_END

		bool _update(sciter::string args) {
#ifndef _DEBUG
		updateClientFiles();
#endif

		this->call_function("self.finish_update");
		return _launch(args);
	}

	bool _launch(sciter::string args) {
		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;
		std::wstring commandLine = L"Diablo II.exe " + args;
		if (CreateProcess(NULL, &commandLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) {
			//WaitForSingleObject(processInfo.hProcess, INFINITE);
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);

			return true;
		}

		return false;
	}

	bool play(sciter::string args) {
		auto fut = std::async(std::launch::async, &frame::_update, this, args);
		pending_futures.push_back(std::move(fut));

		return true;
	}

	bool setLootFilter(sciter::string author, sciter::string filter, sciter::string download_url, sciter::string url) {
		return lootFilter(author, filter, download_url, url);
	}

	std::vector<std::string> getLocalFiles() {
		fs::path filtersPath = (fs::current_path() / "filters").lexically_normal();
		fs::path localPath = (filtersPath / "local").lexically_normal();

		std::vector<std::string> files = {};

		if (fs::exists(localPath)) {
			for (const auto& entry : fs::directory_iterator(localPath)) {
				files.push_back(entry.path().filename().string());
			}
		}

		return files;
	}

	sciter::value getDdrawOptions() {
		return getDdrawIni();
	}

	bool setDdrawOptions(sciter::value ddrawoptions) {
		setDdrawIni(ddrawoptions);
		return true;
	}

private:
	std::vector<std::future<bool>> pending_futures;
};

int uimain(std::function<int()> run) {
	// TODO: Check for sciter.dll
	// enable debug mode
#ifdef _DEBUG
	SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);
#endif

	// ensure only one running instance
	pd2Mutex = CreateMutex(NULL, TRUE, L"pd2.launcher.mutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		MessageBox(NULL, L"The Project Diablo 2 Launcher is already running! Please close it before running it again.", L"Already running!", MB_OK | MB_ICONERROR);
		return 0;
	}

	checkLootFilterFileStructure();

	// needed for persistant storage
	SciterSetOption(NULL, SCITER_SET_SCRIPT_RUNTIME_FEATURES, ALLOW_FILE_IO | ALLOW_SOCKET_IO | ALLOW_EVAL | ALLOW_SYSINFO);

	sciter::archive::instance().open(aux::elements_of(resources)); // bind resources[] (defined in "resources.cpp") with the archive
	sciter::om::hasset<frame> pwin = new frame();

	// note: this:://app URL is dedicated to the sciter::archive content associated with the application
	pwin->load(WSTR("this://app/main.htm"));
	pwin->expand();

	// If beta, point the buckets to the beta launcher
#ifdef BETA_LAUNCHER
	LAUNCHER_BUCKET = BETA_LAUNCHER_BUCKET;
	CLIENT_FILES_BUCKET = BETA_CLIENT_FILES_BUCKET;
#endif

#ifndef _DEBUG
	updateLauncher();
#endif

	// start the launcher ui
	int result = run();

	// close the mutex
	if (pd2Mutex) {
		ReleaseMutex(pd2Mutex);
		CloseHandle(pd2Mutex);
	}

	return result;
}
