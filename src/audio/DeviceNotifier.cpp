#include "DeviceNotifier.h"

#include "../util/Logger.h"

namespace rv::audio {

DeviceNotifier::DeviceNotifier(Callback callback) : callback_(std::move(callback)) {}

HRESULT DeviceNotifier::Register(IMMDeviceEnumerator* enumerator) {
    if (enumerator == nullptr) {
        return E_POINTER;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    enumerator_ = enumerator;
    return enumerator_->RegisterEndpointNotificationCallback(this);
}

void DeviceNotifier::Unregister() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (enumerator_) {
        enumerator_->UnregisterEndpointNotificationCallback(this);
        enumerator_.Reset();
    }
}

ULONG STDMETHODCALLTYPE DeviceNotifier::AddRef() {
    return ++refCount_;
}

ULONG STDMETHODCALLTYPE DeviceNotifier::Release() {
    const ULONG count = --refCount_;
    if (count == 0) {
        delete this;
    }
    return count;
}

HRESULT STDMETHODCALLTYPE DeviceNotifier::QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == nullptr) {
        return E_POINTER;
    }

    if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
        *ppvObject = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE DeviceNotifier::OnDefaultDeviceChanged(
    const EDataFlow flow,
    const ERole role,
    LPCWSTR pwstrDefaultDeviceId) {
    if (flow == eRender && role == eConsole && callback_) {
        std::wstring id = pwstrDefaultDeviceId != nullptr ? pwstrDefaultDeviceId : L"";
        util::Logger::Instance().Info("Default render endpoint changed; scheduling capture restart.");
        callback_(id);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotifier::OnDeviceAdded(LPCWSTR) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotifier::OnDeviceRemoved(LPCWSTR) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotifier::OnDeviceStateChanged(LPCWSTR, DWORD) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotifier::OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) {
    return S_OK;
}

} // namespace rv::audio
