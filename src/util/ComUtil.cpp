#include "ComUtil.h"

#include <comdef.h>
#include <cstdio>

namespace rv::util {

std::string HResultToString(const HRESULT hr) {
    _com_error err(hr);
    const wchar_t* message = err.ErrorMessage();

    char hrHex[16]{};
    std::snprintf(hrHex, sizeof(hrHex), "0x%08X", static_cast<unsigned>(hr));

    std::string result = hrHex;
    result += " (";
    result += WStringToUtf8(message);
    result += ")";
    return result;
}

std::string WStringToUtf8(const wchar_t* text) {
    if (text == nullptr) {
        return {};
    }

    const int needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }

    std::string utf8(static_cast<size_t>(needed - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data(), needed, nullptr, nullptr);
    return utf8;
}

ScopedCoInitialize::ScopedCoInitialize() : hr_(CoInitializeEx(nullptr, COINIT_MULTITHREADED)), uninitialize_(false) {
    if (SUCCEEDED(hr_)) {
        uninitialize_ = true;
    } else if (hr_ == RPC_E_CHANGED_MODE) {
        // COM already initialized with a different apartment model in this thread.
        // We can continue, but must not call CoUninitialize in this case.
    }
}

ScopedCoInitialize::~ScopedCoInitialize() {
    if (uninitialize_) {
        CoUninitialize();
    }
}

} // namespace rv::util
