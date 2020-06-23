// Written by Hasan Shadi Amkieh, last changed 23/06/2020 at 3:24 PM

#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>
#include <stdexcept>
#include "dirent.h"

using namespace std;

struct PluralsightVersion {

	int versionPart1 = 0, versionPart2 = 0, versionPart3 = 0;
	// v1.v2.v3

};

unsigned char equals(char const* str1, char const* str2) {

	for (int i = 0; ; i++) {
		if (str1[i] == str2[i]) {
			if (!str1[i]) {
				return 1;
			}
		}
		else {
			return 0;
		}
	}
}

unsigned char isPluralsightRunning() noexcept {

	char* title = NULL;
	for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
	{
		if (title != NULL) { delete[] title; title = NULL; }

		if (!IsWindowVisible(hwnd))
			continue;

		int length = GetWindowTextLength(hwnd);
		if (length == 0)
			continue;

		title = new char[length + 1];
		GetWindowTextA(hwnd, title, length + 1);

		if (equals(title, "Program Manager")) continue;

		if (equals(title, "Pluralsight Offline Player") || equals(title, "Pluralsight (32 bit)")) {
			printf("\nThe app is still running!\n\n");
			return true;
		}

	}
	if (title != NULL) { delete[] title; title = NULL; }

	printf("\nThe app is not running!\n\n");
	return false;

}

int countLines(string toCount) {

	return count(toCount.begin(), toCount.end(), '\n');

}

std::string* execCommand(const char* cmd) {
	char buffer[128];
	std::string* result = new string;
	FILE* pipe = _popen(cmd, "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (fgets(buffer, sizeof buffer, pipe) != NULL) {
			*result += buffer;
		}
	}
	catch (...) {
		_pclose(pipe);
		throw;
	}
	_pclose(pipe);
	return result;
}

vector<string> divideString(const string toDivide, string delimiter) {

	vector<string> result; //result.reserve(countLines(toDivide));

	size_t last = 0; size_t next = 0;
	while ((next = toDivide.find(delimiter, last)) != string::npos) {
		result.push_back(toDivide.substr(last, next - last));
		last = next + 1;
	}
	result.push_back(toDivide.substr(last));

	return result;

}

vector<string>* getListOfDirs(string path) {

	vector<string>* result = new vector<string>();
	result->reserve(10); // performance reasons..

	printf("Going to search inside: %s\n", path.c_str());
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(path.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (!(equals(ent->d_name, ".") || equals(ent->d_name, ".."))) {
				result->push_back(ent->d_name);
			}
		}
		closedir(dir);
	}
	else {
		perror("An error has occured inside getListOfDirs");
	}

	return result;

}

void copyVersions(int v1, int v2, int v3, PluralsightVersion& dst) {

	dst.versionPart1 = v1;
	dst.versionPart2 = v2;
	dst.versionPart3 = v3;

}

int main(void) noexcept {

	// uncomment this line and comment out the next line to enable the console and see the details: 
	//ShowWindow(GetConsoleWindow(), SW_SHOW);
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	DWORD nameSize = 50;
	char currentUserName[50] = { 0 };
	char currentUserNameInAppPath[50] = { 0 };
	{ // fill the paths with data
		GetUserNameA(currentUserName, &nameSize);
		char programPath[255];
		GetModuleFileNameA(NULL, programPath, 255);
		string programPathStr(programPath);
		int beginOfUserName = programPathStr.find('\\', 3) + 1; // 9
		strcpy(currentUserNameInAppPath,
			programPathStr.substr(beginOfUserName, programPathStr.find('\\', beginOfUserName) - beginOfUserName).c_str());
	}

	int indexNumber = -1;
	{ // get the index number 
		string* result = execCommand("wmic nic get NetConnectionStatus, Index");
		//printf("Current network adapters: \n%s\n\n\n", *result);
		int currentLineIndex = 0;
		vector<string> lines = divideString(*result, "\n");

		auto strIter = begin(lines);
		while (strIter != end(lines)) {
			*strIter = strIter->substr(0, 8);
			strIter++;
		}

		for (string line : lines) {
			if (line.at(line.length() - 1) == '2') {
				indexNumber = atoi(line.substr(0, 3).c_str());
				break;
			}
		}

		if (indexNumber == -1) printf("There is not any network adapter connected to any network!\n");
		else printf("There is a network adapter to disable with indexNumber: %d\n",
			indexNumber);
	}

	HINSTANCE errorCode;
	if (indexNumber != -1) { // disable NIC.
		char toExecute[512];
		strcpy(toExecute, "path win32_networkadapter where index=");
		char indexNumberStr[3];
		_itoa(indexNumber, indexNumberStr, 10);
		strcat(toExecute, indexNumberStr);
		strcat(toExecute, " call disable");

		if ((errorCode = ShellExecuteA(NULL, "runas",
			"wmic.exe",
			toExecute,
			NULL,
			SW_HIDE
			)) > (HINSTANCE)32) printf("Disabled the NIC successedly with code %d!\n", errorCode);
		else printf("Failed of disabling the NIC with %d code!\n", errorCode);
	}

	Sleep(3000);

	string versionToRun = "";
	{ // choose the latest pluralsight version
		PluralsightVersion latestVersion;
		bool hasTakenAnotherPath = true;
		do {
			char pathToInvestigate[256] = "C:/Users/";
			strcat(pathToInvestigate, currentUserName);
			strcat(pathToInvestigate, "/AppData/Local/Pluralsight/");
			vector<string>* listOfDirs = getListOfDirs(pathToInvestigate);

			for (string dir : *listOfDirs) { // app-1.0.247 (sample)
				if (!dir.find("app")) {
					auto version = divideString(dir.substr(4), ".");
					int v1 = atoi(version.at(0).c_str());
					int v2 = atoi(version.at(1).c_str());
					int v3 = atoi(version.at(2).c_str());
					if (v1 > latestVersion.versionPart1 || v2 > latestVersion.versionPart2 || v3 > latestVersion.versionPart3) {
						copyVersions(v1, v2, v3, latestVersion);
					}
				}
			}
			delete listOfDirs;

			if (latestVersion.versionPart1 > 0) {
				hasTakenAnotherPath = false;
				printf("The verion is valid! Continuing!\n");
			}
			else {
				printf("The verion is NOT valid! Looping back again!\n");
				if (equals(currentUserName, currentUserNameInAppPath)) {
					printf("There are not not any alternatives!\nCould not find Pluralsight Offline Player!!!\nExiting the program!\n");
					exit(100);
				}
				else {
					strcpy(currentUserName, currentUserNameInAppPath);
				}
			}
		} while(hasTakenAnotherPath);

		char n1[3];
		char n2[3];
		char n3[3];
		_itoa(latestVersion.versionPart1, n1, 10);
		_itoa(latestVersion.versionPart2, n2, 10);
		_itoa(latestVersion.versionPart3, n3, 10);

		versionToRun += "app-";
		versionToRun += n1;
		versionToRun += ".";
		versionToRun += n2;
		versionToRun += ".";
		versionToRun += n3;
		printf("The final version to run is: \"%s\"\n", versionToRun.c_str());
		//delete[] n1, n2, n3;
	}
	printf("123\n");
	{ // run pluralsight
		char appToRun[512];
		strcpy(appToRun, "C:/Users/");
		strcat(appToRun, currentUserName);
		strcat(appToRun, "/AppData/Local/Pluralsight/"); // should be "app-1.0.247" as a result
		strcat(appToRun, versionToRun.c_str());
		strcat(appToRun, "/Pluralsight.exe");
		/* Note: If the program runs the default Pluralsight.exe inside the pluralsight file,
		this will cause it to glitch and not to run properly, so choose a version file and run the *.exe that is
		inside it!
		*/
		printf("Running \"%s\"\n", appToRun); // sample: [C:\Users\Hassan\AppData\Local\Pluralsight\app-1.0.247\Pluralsight.exe]
		ShellExecuteA(NULL, "open",
			appToRun,
			NULL, NULL, NULL);

		while (!isPluralsightRunning()) {
			Sleep(10000);
			printf("Program is closed! Waiting for it to get it opened!\n");
		}

		while (true) {

			if (isPluralsightRunning()) {
				Sleep(10000);
				continue;
			}
			else break;

		}

	}

	if (indexNumber != -1) {
		char toExecute[512];
		strcpy(toExecute, "path win32_networkadapter where index=");
		char indexNumberStr[3];
		_itoa(indexNumber, indexNumberStr, 10);
		strcat(toExecute, indexNumberStr);
		strcat(toExecute, " call enable");

		if ((errorCode = ShellExecuteA(NULL, "runas",
			"wmic.exe",
			toExecute,
			NULL,
			SW_HIDE
			)) > (HINSTANCE)32) printf("enabled the NIC successfully with return code %d!\n", errorCode);
		else printf("Failed of enabling the NIC with %d return code!\n", errorCode);
	}
}

// wmic nic get Name, NetConnectionStatus, index
// use this command to search for ANY adapters that are conencted to the internet and disconnect them all!

/*

 * possible numbers for NetConnectionStatus and their meanings:

0 = Disconnected

1 = Connecting

2 = Connected 2

3 = Disconnecting

4 = Hardware Not Present

5 = Hardware Disabled

6 = Hardware Malfunction

7 = Media Disconnected

8 = Authenticating

9 = Authentication Succeeded

10 = Authentication Failed

11 = Invalid Address

12 = Credentials Required

*/

