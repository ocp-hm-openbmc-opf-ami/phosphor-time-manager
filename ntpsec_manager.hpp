#pragma once

#include <sdbusplus/asio/object_server.hpp>
#include <string>

enum class ServiceAction { Start, Stop, Restart };

void registerNTPSecDbus(std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceNTPSecConf);
