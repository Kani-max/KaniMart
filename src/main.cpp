#include <drogon/drogon.h>
#include <iostream>

int main()
{
    try
    {
        std::cout << "[1] KaniMart starting..." << std::endl;

        auto& app = drogon::app();

        app.setLogLevel(trantor::Logger::kInfo);

        std::cout << "[2] Loading configuration..." << std::endl;

        app.loadConfigFile("config.json");

        std::cout << "[3] Starting Drogon..." << std::endl;

        app.run();

        std::cout << "[4] Drogon stopped." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "[ERROR] Unknown exception." << std::endl;
        return 2;
    }

    return 0;
}