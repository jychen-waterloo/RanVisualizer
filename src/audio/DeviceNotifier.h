#pragma once

#include <Mmdeviceapi.h>
#include <wrl/client.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>

namespace rv::audio {

class DeviceNotifier final : public IMMNotificationClient {
public:
    using Callback = std::function<void(const std::wstring& endpointId)>;

    explicit DeviceNotifier(Callback callback);

    HRESULT Register(IMMDeviceEnumerator* enumerator);
    void Unregister();

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

    // IMMNotificationClient
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override;
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override;

private:
    std::atomic<ULONG> refCount_{1};
    Callback callback_;
    std::mutex mutex_;
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator_;
};

} // namespace rv::audio
