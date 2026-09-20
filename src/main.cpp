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
        // Allow the local frontend to communicate with the Drogon API.
        app.registerPreSendingAdvice(
            [](const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response)
            {
                const auto origin = request->getHeader("Origin");
                if (origin == "http://127.0.0.1:5500" ||
                    origin == "http://localhost:5500")
                {
                    response->addHeader(
                        "Access-Control-Allow-Origin",
                        origin);
                    response->addHeader(
                        "Access-Control-Allow-Methods",
                        "GET, POST, PUT, DELETE, OPTIONS");
                    response->addHeader(
                        "Access-Control-Allow-Headers",
                        "Content-Type, Authorization");
                    response->addHeader(
                        "Access-Control-Allow-Credentials",
                        "true");
                }
                return response;
            });
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
