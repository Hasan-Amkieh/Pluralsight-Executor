#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>

using namespace std;

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

		std::cout << "HWND: " << hwnd << " Title: " << title << std::endl;
		//for (int i = 0 ; i != length+1 ; i++) {
		//	printf("%c", title[i]);
		//}

		if (equals(title, "Pluralsight Offline Player") || equals(title, "Pluralsight (32 bit)")) {
			printf("\nThe app is still running!\n\n");
			return true;
		}

	}
	if (title != NULL) { delete[] title; title = NULL; }

	printf("\nThe app is not running!\n\n");
	return false;

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

	vector<string> result;

	size_t last = 0; size_t next = 0;
	while ((next = toDivide.find(delimiter, last)) != string::npos) {
		result.push_back(toDivide.substr(last, next - last));
		last = next + 1;
	}
	result.push_back(toDivide.substr(last));

	return result;

}

int main(void) noexcept {

	// uncomment this line and comment out the next line to enable the console and see the details: 
	//ShowWindow(GetConsoleWindow(), SW_SHOW); 
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	DWORD nameSize = 50;
	char currentUserName[50] = { 0 };
	GetUserNameA(currentUserName, &nameSize);

	int indexNumber = -1;
	{ // get the index number 
		string* result = execCommand("wmic nic get NetConnectionStatus, Index");
		//printf("Current network adapters: \n%s\n\n\n", *result);
		int currentLineIndex = 0;
		vector<string> lines = divideString(*result, "\n");

		auto strIter = begin(lines);
		while (strIter != end(lines)) {
			*strIter = strIter->substr(0, 8);
			printf("Truncated to: %s\n", strIter->c_str());
			strIter++;
		}

		for (string line : lines) {
			if (line.at(line.length() - 1) == '2') {
				indexNumber = atoi(line.substr(0, 3).c_str());
				printf("Found an index number of: %d\n", indexNumber);
				break;
			}
		}

		if (indexNumber == -1) printf("There is not any network adapter connected to any network!\n");
		else printf("There is a network adapter to disable with indexNumber: %d\n",
			indexNumber);
	}

	HINSTANCE errorCode;
	if (indexNumber != -1) { // disable NIC
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

	{ // run pluralsight
		char appToRun[512];
		strcpy(appToRun, "C:/Users/");
		strcat(appToRun, currentUserName);
		strcat(appToRun, "/AppData/Local/Pluralsight/app-1.0.242/Pluralsight.exe");

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
