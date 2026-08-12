#include <drogon/drogon.h>
#include <iostream>

int main()
{
    std::cout << "[1] KaniMart starting..." << std::endl;

    auto& app = drogon::app();

    app.setLogLevel(trantor::Logger::kInfo);

    std::cout << "[2] Adding HTTP listener..." << std::endl;

    app.addListener("127.0.0.1", 8080);

    std::cout << "[3] Starting Drogon..." << std::endl;

    app.run();

    std::cout << "[4] Drogon stopped." << std::endl;

    return 0;
}