#include <drogon/drogon.h>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

bool isAllowedOrigin(const std::string& origin)
{
    const char* configuredOrigins =
        std::getenv("KANIMART_ALLOWED_ORIGINS");

    if (configuredOrigins == nullptr ||
        std::string(configuredOrigins).empty())
    {
        return origin == "http://127.0.0.1:5500" ||
            origin == "http://localhost:5500";
    }

    const std::string origins(configuredOrigins);
    std::size_t start = 0;

    while (start < origins.size())
    {
        const std::size_t end = origins.find(',', start);
        const std::string candidate = origins.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);

        if (candidate == origin)
        {
            return true;
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return false;
}

} // namespace

int main()
{
    try
    {
        std::cout << "[1] KaniMart starting..." << std::endl;
        auto& app = drogon::app();
        app.setLogLevel(trantor::Logger::kInfo);
        std::cout << "[2] Loading configuration..." << std::endl;
        const char* configFile =
            std::getenv("KANIMART_CONFIG_FILE");

        app.loadConfigFile(
            configFile == nullptr || std::string(configFile).empty()
                ? "config.json"
                : configFile);

        app.setDocumentRoot("frontend");

        app.registerPreSendingAdvice(
            [](const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response)
            {
                const auto origin = request->getHeader("Origin");
                if (isAllowedOrigin(origin))
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
