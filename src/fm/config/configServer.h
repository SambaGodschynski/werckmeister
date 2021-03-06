#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

#include <unordered_map>
#include <string>
#include <vector>
#include "deviceConfig.h"
#include <fm/common.hpp>

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <loki/Singleton.h>
#pragma GCC diagnostic pop
#else
#include <loki/Singleton.h>
#endif

namespace fm {
    class ConfigServer {
    friend struct Loki::CreateUsingNew<ConfigServer>;
    public:
        typedef std::unordered_map<fm::String, DeviceConfig> Devices;
        ConfigServer();
		ConfigServer(const ConfigServer&&) = delete;
		ConfigServer& operator=(const ConfigServer&&) = delete;
        ~ConfigServer();
        DeviceConfig createMidiDeviceConfig(const fm::String &uname, const DeviceConfig::DeviceId &deviceId, int offsetMillis = 0);
        DeviceConfig createFluidSynthDeviceConfig(const fm::String& uname, const std::string &soundfontFile, int offsetMillis = 0);
        void addDevice(const DeviceConfig &config);
        void clear();
        const DeviceConfig * getDevice(const fm::String &name) const;
        const Devices & getDevices() const { return devices; }
    private:
        Devices devices;
    };
    ConfigServer & getConfigServer();
}
#endif