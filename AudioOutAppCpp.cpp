#include <iostream>
#include <fstream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <endpointvolume.h>
#include "json.hpp"
#include "PolicyConfig.h"


nlohmann::json ParseConfig() {
    std::ifstream cfgin("config.json");
    nlohmann::json config = nlohmann::json::parse(cfgin);
    cfgin.close();
    return config;
}

void WriteConfig(nlohmann::json config) {
    std::string s = config.dump(4);
    std::ofstream configFile;
    configFile.open("config.json");
    configFile << s;
    configFile.close();
}

// Set active device
HRESULT RegisterDevice(LPCWSTR devID, ERole role) {
    IPolicyConfig* pPolicyConfig = nullptr;

    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig), (LPVOID*)&pPolicyConfig);
    if (pPolicyConfig == nullptr) {
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig10), (LPVOID*)&pPolicyConfig);
    }
    if (pPolicyConfig == nullptr) {
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig7), (LPVOID*)&pPolicyConfig);
    }
    if (pPolicyConfig == nullptr) {
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID*)&pPolicyConfig);
    }
    if (pPolicyConfig == nullptr) {
        hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig10_1), (LPVOID*)&pPolicyConfig);
    }

    if (pPolicyConfig != NULL) {
        hr = pPolicyConfig->SetDefaultEndpoint(devID, role);
        pPolicyConfig->Release();
    }

    return hr;
}

void ToggleOutputDevice() {
    HRESULT hr;
    hr = CoInitialize(NULL);
    IMMDevice* defaultDevice = NULL;
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDeviceCollection* pCollection = NULL;
    IPropertyStore* pProps = NULL;
    LPWSTR pwszID = NULL;
    LPWSTR defaultpwszID = NULL;
    UINT count = 0;
    float currentVolume = 0;

    // Read configuration file
    nlohmann::json config = ParseConfig();
    // Todo: check if config file exists, if not, print a list of devices or create a dummy/default devices?

    // Read device ID and current volume of currently active output device
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    defaultDevice->GetId(&defaultpwszID);
    IAudioEndpointVolume* endpointVolume = NULL;
    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&endpointVolume);
    hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
    std::wstring widestring(defaultpwszID);
    std::string key = std::string(widestring.begin(), widestring.end());
    std::cout << "Current device: " << key << std::endl;

    // Store current volume
    config["cacheVolume"][key] = currentVolume;
    defaultDevice->Release();
    defaultDevice = NULL;

    // Reorder cycle list in config, so that current device is placed to the end.
    auto pos = std::find(config["cycle"].begin(), config["cycle"].end(), key);
    if (pos == config["cycle"].end()) {
        std::cout << "Current audio device not in cycle list. Will just skip to first device in cycle list." << std::endl;
    } else {
        std::cout << "Moving current audio device to end of cycle list." << std::endl;
        config["cycle"].erase(pos);
        config["cycle"].push_back(key);
    }
    std::cout << "Next Device: " << std::endl;
    std::cout << "\t" << config["cycle"].at(0) << std::endl;

    // Get a list of all active audio devices
    hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (pCollection && FAILED(pCollection->GetCount(&count)))
        count = 0;

    // Iterate over devices...
    for (ULONG i = 0; i < count; ++i) {
        IMMDevice* pEndpoint = 0;
        pCollection->Item(i, &pEndpoint);
        if (pEndpoint) {
            pEndpoint->GetId(&pwszID);
            /* // Read "friendly name", useful for debugging/initializing config list.
            hr = pEndpoint->OpenPropertyStore(
                STGM_READ, &pProps);
            PROPVARIANT varName;
            PropVariantInit(&varName);
            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
            printf("Endpoint %d: \"%S\" (%S)\n", i, varName.pwszVal, pwszID);
            */

            // Get this device id and check if it's next
            std::wstring widestring2(pwszID);
            std::string key2 = std::string(widestring2.begin(), widestring2.end());
            bool isNext = key2.compare(config["cycle"].at(0)) == 0;

            if (isNext) {
                // Mute current device
                hr = endpointVolume->SetMasterVolumeLevelScalar(0.0, NULL);
                endpointVolume->Release();

                // Activate this next sound device
                RegisterDevice(pwszID, eConsole);
                hr = pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&endpointVolume);
                float cacheVolume = config["cacheVolume"][key2];
                hr = endpointVolume->SetMasterVolumeLevelScalar((float)cacheVolume, NULL);
                std::cout << "setting device volume to stored value: " << cacheVolume << std::endl;
                endpointVolume->Release();
            }

            pEndpoint->Release();
        }
    }
    deviceEnumerator->Release();
    deviceEnumerator = NULL;
    WriteConfig(config);
}

int main() {
    ToggleOutputDevice();
    return 0;
}
