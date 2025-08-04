#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>
#include <comdef.h>
#include <strsafe.h>

//#include <shlwapi.h>
//#pragma comment(lib, "shlwaspi.lib")

#include "hrlog.h"

#ifdef _DEBUG
static const bool hrlog_debug = true;
#else
static const bool hrlog_debug = false;
#endif



class HrLog {
private:
    HANDLE hFile;

    HrLog() : hFile(INVALID_HANDLE_VALUE) {

        // Get executable path and create log file path
        wchar_t exePath[MAX_PATH];
        wchar_t logPath[MAX_PATH];

        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        StringCchCopyW(logPath, MAX_PATH, exePath);
        StringCchCatW(logPath, MAX_PATH, L".log");

        // Create/open log file once
        hFile = CreateFileW(
            logPath,
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,  // Start fresh each run
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    ~HrLog() {
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        }
    }

public:
    static HrLog& Instance() {
        static HrLog instance;
        return instance;
    }

    static _bstr_t make_error_msg(HRESULT hr)
    {
        _bstr_t errorMsg;

        if (FAILED(hr)) {
            // Get COM error message
            _com_error err(hr);
            // Try Description() first (more detailed), fallback to ErrorMessage()
            _bstr_t description = err.Description();
            if (description.length() > 0) {
                errorMsg = description;
            }
            else {
                errorMsg = err.ErrorMessage();
            }
        }
        else {
            // For successful HRESULTs, use a generic success message
            errorMsg = L"Operation succeeded";
        }
        return errorMsg;
    }

    void Log(HRESULT hr, int line_number, const char* prompt_) {
        // Only log success messages in debug mode
        if (!hrlog_debug || hFile == INVALID_HANDLE_VALUE) {
            return;
        }

        // Get current time
        SYSTEMTIME st;
        GetLocalTime(&st);

        // Format timestamp
        char timestamp[64];
        StringCchPrintfA(timestamp, 64, "[%04d-%02d-%02d %02d:%02d:%02d.%03d]",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        // Get error message
		// hold it in a _bstr_t instance explicitly
		_bstr_t errorMsgBSTR = make_error_msg(hr);
		// this is just a pointer into the BSTR data
		// ditto we need to keep that BSTR alive
        const char* statusMsg = (const char*)errorMsgBSTR;

        // Format complete log entry
        char logEntry[1024] = {};
        StringCchPrintfA(logEntry, 1024, "%s Line %d: %s - %s\r\n",
            timestamp, line_number, prompt_, statusMsg);

        // Write directly as ASCII
        DWORD bytesWritten;
        WriteFile(hFile, logEntry, strlen(logEntry), &bytesWritten, NULL);
        FlushFileBuffers(hFile);  // Ensure immediate write
    }
};

// visible to callers
bool hrlog(HRESULT hr, int line_number, const char* prompt_, ...) {
    // logging only while debugging
    if (hrlog_debug) {
        va_list args;
        va_start(args, prompt_);
        // Format the prompt first
        char formattedPrompt[512] = {};
        StringCchVPrintfA(formattedPrompt, 512, prompt_, args);
        va_end(args);
        HrLog::Instance().Log(hr, line_number, formattedPrompt);
    }
    return SUCCEEDED(hr) ? true : false;
}

//#define HRLOG(hr, ...) hrlog(hr, __LINE__, __VA_ARGS__)