#pragma once

#include <Windows.h>

#include <string>

namespace rv::util {

std::string HResultToString(HRESULT hr);
std::string WStringToUtf8(const wchar_t* text);

class ScopedCoInitialize {
public:
    ScopedCoInitialize();
    ~ScopedCoInitialize();

    ScopedCoInitialize(const ScopedCoInitialize&) = delete;
    ScopedCoInitialize& operator=(const ScopedCoInitialize&) = delete;

    [[nodiscard]] HRESULT Result() const { return hr_; }

private:
    HRESULT hr_;
    bool uninitialize_;
};

} // namespace rv::util
