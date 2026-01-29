#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include "ntpsec_manager.hpp"
#include <iostream>

int main() 
{
    try {
        boost::asio::io_context io;
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);

        sdbusplus::server::manager_t objManager(*conn, "/xyz/openbmc_project/NTPsec");
        conn->request_name("xyz.openbmc_project.NTPsec.Config");

        auto server = sdbusplus::asio::object_server(conn);
        auto iface = server.add_interface("/xyz/openbmc_project/NTPsec", "xyz.openbmc_project.NTPsec.Config");

        registerNTPSecDbus(iface);
        io.run();
    } 
    catch (const sdbusplus::exception::SdBusError& e) {
        std::cerr << "Failed to run Secure NTP service manager " << e.what() << '\n';
        return false;
    }

    return 0;
}
