#include <drogon/drogon.h>
#include <drogon/orm/DbConfig.h>

#include <json/json.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

bool isAllowedOrigin(const std::string& origin)
{
    const char* configuredOrigins =
        std::getenv("KANIMART_ALLOWED_ORIGINS");

    if (origin.empty() ||
        configuredOrigins == nullptr ||
        std::string(configuredOrigins).empty())
    {
        return false;
    }

    std::string allowedOrigins(configuredOrigins);

    std::size_t start = 0;

    while (start < allowedOrigins.length())
    {
        std::size_t end = allowedOrigins.find(',', start);

        if (end == std::string::npos)
        {
            end = allowedOrigins.length();
        }

        std::string allowed =
            allowedOrigins.substr(start, end - start);

        if (allowed == origin)
        {
            return true;
        }

        start = end + 1;
    }

    return false;
}

bool isProduction()
{
    const char* databaseUrl =
        std::getenv("DATABASE_URL");

    return databaseUrl != nullptr &&
           std::string(databaseUrl).length() > 0;
}

bool configureProductionDatabase()
{
    const char* databaseUrl =
        std::getenv("DATABASE_URL");

    if (databaseUrl == nullptr ||
        std::string(databaseUrl).empty())
    {
        std::cerr
            << "[ERROR] DATABASE_URL is not configured."
            << std::endl;

        return false;
    }

    std::string url(databaseUrl);

    // Remove query parameters such as ?sslmode=require
    const std::size_t queryPos = url.find('?');

    if (queryPos != std::string::npos)
    {
        url = url.substr(0, queryPos);
    }

    std::string prefix;

    if (url.rfind("postgresql://", 0) == 0)
    {
        prefix = "postgresql://";
    }
    else if (url.rfind("postgres://", 0) == 0)
    {
        prefix = "postgres://";
    }
    else
    {
        std::cerr
            << "[ERROR] DATABASE_URL must start with "
            << "postgresql:// or postgres://"
            << std::endl;

        return false;
    }

    url.erase(0, prefix.length());

    const std::size_t atPos = url.rfind('@');
    const std::size_t colonPos = url.find(':');

    if (atPos == std::string::npos ||
        colonPos == std::string::npos ||
        colonPos > atPos)
    {
        std::cerr
            << "[ERROR] Invalid DATABASE_URL format."
            << std::endl;

        return false;
    }

    const std::string username =
        url.substr(0, colonPos);

    const std::string password =
        url.substr(
            colonPos + 1,
            atPos - colonPos - 1);

    const std::string hostPortDatabase =
        url.substr(atPos + 1);

    const std::size_t slashPos =
        hostPortDatabase.find('/');

    if (slashPos == std::string::npos)
    {
        std::cerr
            << "[ERROR] DATABASE_URL has no database name."
            << std::endl;

        return false;
    }

    const std::string hostPort =
        hostPortDatabase.substr(0, slashPos);

    const std::string databaseName =
        hostPortDatabase.substr(slashPos + 1);

    const std::size_t hostColonPos =
        hostPort.rfind(':');

    if (hostColonPos == std::string::npos)
    {
        std::cerr
            << "[ERROR] DATABASE_URL has no port."
            << std::endl;

        return false;
    }

    const std::string host =
        hostPort.substr(0, hostColonPos);

    unsigned short port;

    try
    {
        port = static_cast<unsigned short>(
            std::stoi(
                hostPort.substr(hostColonPos + 1)));
    }
    catch (...)
    {
        std::cerr
            << "[ERROR] Invalid PostgreSQL port."
            << std::endl;

        return false;
    }

    drogon::orm::PostgresConfig dbConfig;

    dbConfig.host = host;
    dbConfig.port = port;
    dbConfig.databaseName = databaseName;
    dbConfig.username = username;
    dbConfig.password = password;
    dbConfig.connectionNumber = 4;
    dbConfig.name = "kanimart";
    dbConfig.isFast = false;
    dbConfig.characterSet = "";
    dbConfig.timeout = 10.0;
    dbConfig.autoBatch = false;

    drogon::app().addDbClient(dbConfig);

    std::cout
        << "[DB] Production PostgreSQL configured."
        << std::endl;

    return true;
}

bool loadProductionConfig()
{
    Json::Value config;

    std::ifstream file("config.json");

    if (!file.is_open())
    {
        std::cerr
            << "[ERROR] Could not open config.json."
            << std::endl;

        return false;
    }

    file >> config;
    file.close();

    /*
     * Keep the existing listeners from config.json.
     * If Render provides PORT, use it.
     */
    const char* renderPort =
        std::getenv("PORT");

    if (renderPort != nullptr &&
        std::string(renderPort).length() > 0)
    {
        try
        {
            config["listeners"][0]["port"] =
                std::stoi(renderPort);
        }
        catch (...)
        {
            std::cerr
                << "[ERROR] Invalid PORT value."
                << std::endl;

            return false;
        }
    }

    /*
     * Remove the local database configuration.
     * The real production database is registered
     * separately from DATABASE_URL.
     */
    config.removeMember("db_clients");

    drogon::app().loadConfigJson(config);

    return configureProductionDatabase();
}

} // namespace

int main()
{
    try
    {
        std::cout
            << "[1] KaniMart starting..."
            << std::endl;

        auto& app = drogon::app();

        app.setLogLevel(
            trantor::Logger::kInfo);

        std::cout
            << "[2] Loading configuration..."
            << std::endl;

        if (isProduction())
        {
            std::cout
                << "[DB] DATABASE_URL detected."
                << std::endl;

            if (!loadProductionConfig())
            {
                std::cerr
                    << "[ERROR] Production configuration failed."
                    << std::endl;

                return 1;
            }
        }
        else
        {
            const char* configFile =
                std::getenv("KANIMART_CONFIG_FILE");

            app.loadConfigFile(
                configFile == nullptr ||
                        std::string(configFile).empty()
                    ? "config.json"
                    : configFile);

            std::cout
                << "[DB] Using local config.json."
                << std::endl;
        }

        app.setDocumentRoot("frontend");

        app.registerPreSendingAdvice(
            [](const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response)
            {
                const auto origin =
                    request->getHeader("Origin");

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

        std::cout
            << "[3] Starting Drogon..."
            << std::endl;

        app.run();

        std::cout
            << "[4] Drogon stopped."
            << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[ERROR] "
            << e.what()
            << std::endl;

        return 2;
    }
    catch (...)
    {
        std::cerr
            << "[ERROR] Unknown exception."
            << std::endl;

        return 3;
    }

    return 0;
}