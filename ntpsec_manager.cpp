#include "ntpsec_manager.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <thread>
#include <chrono>

const std::string ntpFilePath = "/etc/ntp.conf";
const std::string tempFilePath = "/etc/temp_ntp.conf";

std::vector<std::string> readServersFromConfig() {
    std::ifstream configFile(ntpFilePath);
    std::vector<std::string> servers;
    std::string line;

    while (std::getline(configFile, line)) {
        if (line.rfind("server -4", 0) == 0) {
            size_t start = line.find_first_not_of(" ", 9);
            size_t end = line.find(' ', start);
            if (start != std::string::npos) {
                servers.push_back(line.substr(start, end - start));
            }
        }
    }
    return servers;
}

void configureNtpServers(const std::vector<std::string>& newServers) {
    if (newServers.size() > 3) {
        std::cerr << "Error: Maximum of 3 NTP servers allowed.\n";
        return;
    }

    std::ifstream inputFile(ntpFilePath);
    std::ofstream tempFile(tempFilePath);

    if (!inputFile || !tempFile) {
        std::cerr << "Error: Cannot open configuration files.\n";
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.find("server -4") != std::string::npos) continue;
        tempFile << line << '\n';
    }

    for (const auto& server : newServers) {
        tempFile << "server -4 " << server << " ca /etc/ntpsec/ntpsec-ca-certificates.crt nts iburst\n";
    }

    inputFile.close();
    tempFile.close();

    if(std::remove(ntpFilePath.c_str()) != 0) 
    {
        std::cerr << "Error: Failed to remove old configuration.\n";
        return;
    }
    if (std::rename(tempFilePath.c_str(), ntpFilePath.c_str()) != 0) 
    {
        std::cerr << "Failed to rename the ntp.conf file.\n";
        return;
    }
}

bool isServiceActive(const std::string& serviceName = "ntpd.service") {
    try {
        auto bus = sdbusplus::bus::new_default();

        auto msg = bus.new_method_call("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                                       "org.freedesktop.systemd1.Manager", "GetUnit");
        msg.append(serviceName);
        auto reply = bus.call(msg);

        sdbusplus::message::object_path path;
        reply.read(path);

        msg = bus.new_method_call("org.freedesktop.systemd1", static_cast<std::string>(path).c_str(),
                                  "org.freedesktop.DBus.Properties", "Get");
        msg.append("org.freedesktop.systemd1.Unit", "ActiveState");
        reply = bus.call(msg);

        std::variant<std::string> activeState;
        reply.read(activeState);

        return std::get<std::string>(activeState) == "active";

    } catch (const sdbusplus::exception::SdBusError& e) {
        std::cerr << "Service status error: " << e.what() << '\n';
        return false;
    }
}

void controlSystemdService(const std::string& serviceName, ServiceAction action) {
    try {
        auto bus = sdbusplus::bus::new_default();
        std::string method;

        switch (action) {
            case ServiceAction::Start: method = "StartUnit"; break;
            case ServiceAction::Stop: method = "StopUnit"; break;
            case ServiceAction::Restart: method = "RestartUnit"; break;
        }

        auto msg = bus.new_method_call("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                                       "org.freedesktop.systemd1.Manager", method.c_str());
        msg.append(serviceName, "replace");
        bus.call_noreply(msg);
    } catch (const sdbusplus::exception::SdBusError& e) {
        std::cerr << "Systemd service control failed: " << e.what() << '\n';
    }
}

void registerNTPSecDbus(std::shared_ptr<sdbusplus::asio::dbus_interface> iface) {
    std::vector<std::string> servers = {};
    bool isActive = isServiceActive();

    iface->register_property(
        "ServerConfig", servers,
        [](const std::vector<std::string>& newServers, std::vector<std::string>& old) {
            if (old == newServers)
            {
                return true;
            }
            bool isActive = isServiceActive();
            configureNtpServers(newServers);

            if(isActive)
            {
                controlSystemdService("ntpd.service", ServiceAction::Restart);
            }
            old = newServers;
            return true;
        },
        [](const auto&) {
            return readServersFromConfig();
        }
    );

    iface->register_property(
        "NTPSecStatus", isActive,
        [](const bool& newStatus, bool& current) {
            if (newStatus == current) return true;
            controlSystemdService("ntpd.service", newStatus ? ServiceAction::Start : ServiceAction::Stop);
	    std::this_thread::sleep_for(std::chrono::seconds(2));
            current = isServiceActive();
            return true;
        }
    );

    iface->initialize();
}
