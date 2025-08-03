#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sal.h>
#include <dshow.h>
#include <commctrl.h>
#include <initguid.h>
#include <objbase.h>
#include <ole2.h>
#include <comdef.h>

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comctl32.lib")

#include "hrlog.h"

// Define necessary GUIDs
const IID IID_ISpecifyPropertyPages = { 0xB196B28B,0xBAB4,0x101A,{0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07} };

int ShowCameraProperties(int deviceIndex) {
    HRESULT hr = 0;
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;
    IMoniker* pMoniker = nullptr;
    ULONG cFetched = 0;
    int result = 0;

    // Initialize COM
    //hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    //if (! HRLOG(hr, "CoInitializeEx")) return 0;

    if (!HRLOG_EXEC(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
		return 0;  // Initialization failed
	}

    // Create device enumerator
    if (!HRLOG_EXEC(
    CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pDevEnum))) goto cleanup;
    // Create device enumerator interface
	//hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

	//if (!HRLOG(hr, "Failed to create device enumerator")) goto cleanup;

    // Create enumerator for video devices
    if (!HRLOG_EXEC(
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0))) 
        goto cleanup;
    
    if (hr == S_FALSE) goto cleanup;  // No devices

    // Find requested device
    for (int i = 0; pEnum->Next(1, &pMoniker, &cFetched) == S_OK; i++) {
        if (i != deviceIndex) {
            pMoniker->Release();
            continue;
        }

        IBaseFilter* pFilter = nullptr;
        ISpecifyPropertyPages* pProp = nullptr;
        IPropertyBag* pPropBag = nullptr;

        // Bind to filter
        if (!HRLOG_EXEC(
            pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&pFilter))) 
        {
            pMoniker->Release();
            continue;
		}
        

        // Get property pages interface
        if (HRLOG_EXEC(
            pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pProp
            ))) 
        {
            // Get device friendly name using _bstr_t for automatic cleanup
            _bstr_t deviceName(L"Camera Properties");
            if (
                HRLOG_EXEC(
                    pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag)
                )
                )
            {
                _variant_t varName;  // Use _variant_t for automatic cleanup
                HRESULT hr = pPropBag->Read(L"FriendlyName", &varName, nullptr);
                if (SUCCEEDED(hr) && varName.vt == VT_BSTR) {
                    deviceName = varName.bstrVal;
                }
                pPropBag->Release();
            }

            // Get property pages
            CAUUID cauuid = { 0 };
            hr = pProp->GetPages(&cauuid);
            if (SUCCEEDED(hr)) {
                // Show property dialog
                OleCreatePropertyFrame(
                    nullptr,                // Parent window
                    30, 30,                 // Position
                    deviceName,             // Dialog title
                    1,                      // Object count
                    (IUnknown**)&pFilter,   // Objects
                    cauuid.cElems,          // Page count
                    cauuid.pElems,          // Page CLSIDs
                    LOCALE_USER_DEFAULT,    // Locale
                    0, nullptr              // Reserved
                );

                // Cleanup
                CoTaskMemFree(cauuid.pElems);
                result = 1;  // Success
            }
            pProp->Release();
        }

        pFilter->Release();
        pMoniker->Release();
        break;
    }

cleanup:
    if (pEnum) pEnum->Release();
    if (pDevEnum) pDevEnum->Release();
    CoUninitialize();
    return result;
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
) {
    // Show camera properties for the first device
    return ShowCameraProperties(0) ? 0 : 1;
}