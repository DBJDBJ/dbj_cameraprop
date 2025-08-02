#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sal.h>
#include <dshow.h>
#include <commctrl.h>

#include <initguid.h>
#include <objbase.h>
#include <ole2.h>

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comctl32.lib")

// Define necessary GUIDs
const IID IID_ISpecifyPropertyPages = { 0xB196B28B,0xBAB4,0x101A,{0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07} };

int ShowCameraProperties(int deviceIndex) {
    HRESULT hr;
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;
    IMoniker* pMoniker = nullptr;
    ULONG cFetched = 0;
    int result = 0;

    // Initialize COM
    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 0;

    // Create device enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pDevEnum);
    if (FAILED(hr)) goto cleanup;

    // Create enumerator for video devices
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (FAILED(hr)) goto cleanup;
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
        hr = pMoniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&pFilter);
        if (FAILED(hr)) {
            pMoniker->Release();
            continue;
        }

        // Get property pages interface
        hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pProp);
        if (SUCCEEDED(hr)) {
            // Get device friendly name
            WCHAR deviceName[256] = L"Camera Properties";
            hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr)) {
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"FriendlyName", &varName, nullptr);
                if (SUCCEEDED(hr) && varName.vt == VT_BSTR) {
                    wcsncpy_s(deviceName, 256, varName.bstrVal, 255);
                    VariantClear(&varName);
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