#define CURL_STATICLIB
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <common/common.h>

namespace fs = std::filesystem;

const char* LAUNCHER_BUCKET = "https://storage.googleapis.com/storage/v1/b/pd2-beta-launcher-update/o";

void checkUpdates(bool forceDownload) {
	std::cout << "Calling API..." << "\n";

	nlohmann::json json = callJsonAPI(LAUNCHER_BUCKET);

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
		if (forceDownload || !fs::exists(path)) {
			std::cout << "Downloading: " << itemName << "\n";
			downloadFile(mediaLink, path.string());
		}
		else if (!compareCRC(path, crcHash)) {
			std::cout << "CRC Mismatch: " << itemName << "\n";
			downloadFile(mediaLink, path.string());
		}
		else {
			std::cout << "Skipping: " << itemName << "\n";
		}
	}
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	bool forceDownload = false;
	bool startLauncherAfter = false;

	// ensure only one running instance
	HANDLE pd2Mutex = CreateMutex(NULL, TRUE, L"pd2.launcher.mutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		MessageBox(NULL, L"The PD2 Launcher is running, please close it before running the updater.", L"Launcher is running!", MB_OK | MB_ICONERROR);
		return 0;
	}

	// create the logger
	std::ofstream file;
	file.open("updater.log");
	std::streambuf* sbuf = std::cout.rdbuf();
	std::cout.rdbuf(file.rdbuf());

	// parse arguments
	int argCount;
	LPWSTR* szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL) {
		MessageBox(NULL, L"Unable to parse arguments", L"Error", MB_OK);
		return 10;
	}

	for (int i = 0; i < argCount; i++) {
		if (!_wcsicmp(szArgList[i], L"/f")) {
			forceDownload = true;
		}
		if (!_wcsicmp(szArgList[i], L"/l")) {
			startLauncherAfter = true;
		}
	}

	LocalFree(szArgList);

	// check for updates to the launcher
	checkUpdates(forceDownload);

	// check if we should launch the launcher
	if (startLauncherAfter) {
		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;
		std::wstring commandLine = L"PD2Launcher.exe";
		if (CreateProcess(NULL, &commandLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) {
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);
		}
	}

	// close the mutex
	if (pd2Mutex) {
		ReleaseMutex(pd2Mutex);
		CloseHandle(pd2Mutex);
	}

	return 0;
}
