#include "AuthController.h"

#include <drogon/drogon.h>
#include <sodium.h>

#include "../services/JwtService.h"

#include <array>
#include <string>

namespace kani::kanimart
{

namespace
{

bool initializeSodium()
{
    return sodium_init() >= 0;
}

bool hashPassword(
    const std::string& password,
    std::string& passwordHash)
{
    std::array<char, crypto_pwhash_STRBYTES> hash{};

    if (crypto_pwhash_str_alg(
            hash.data(),
            password.c_str(),
            password.size(),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE,
            crypto_pwhash_ALG_ARGON2ID13) != 0)
    {
        return false;
    }

    passwordHash = hash.data();
    return true;
}

bool verifyPassword(
    const std::string& passwordHash,
    const std::string& password)
{
    return crypto_pwhash_str_verify(
               passwordHash.c_str(),
               password.c_str(),
               password.size()) == 0;
}

} // namespace

void AuthController::registerUser(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        Json::Value response;

        if (!initializeSodium())
        {
            response["success"] = false;
            response["error"] = "Password security initialization failed";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            callback(resp);
            return;
        }

        auto json = request->getJsonObject();

        if (!json)
        {
            response["success"] = false;
            response["error"] = "Invalid JSON request";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        const std::string name =
            (*json)["name"].asString();

        const std::string email =
            (*json)["email"].asString();

        const std::string password =
            (*json)["password"].asString();

        if (name.empty() ||
            email.empty() ||
            password.empty())
        {
            response["success"] = false;
            response["error"] =
                "Name, email and password are required";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        if (password.size() < 8)
        {
            response["success"] = false;
            response["error"] =
                "Password must be at least 8 characters";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        std::string passwordHash;

        if (!hashPassword(password, passwordHash))
        {
            response["success"] = false;
            response["error"] =
                "Password hashing failed";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            callback(resp);
            return;
        }

        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "INSERT INTO users "
            "(name, email, password_hash, role) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id, name, email, role",
            name,
            email,
            passwordHash,
            "BUYER");

        const auto& row = result[0];

        response["success"] = true;
        response["message"] =
            "Registration successful";

        response["data"]["user"]["id"] =
            row["id"].as<int>();

        response["data"]["user"]["name"] =
            row["name"].as<std::string>();

        response["data"]["user"]["email"] =
            row["email"].as<std::string>();

        response["data"]["user"]["role"] =
            row["role"].as<std::string>();

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(response);

        resp->setStatusCode(drogon::k201Created);

        callback(resp);
    }
    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;

        const std::string error = exception.what();

        if (error.find("users_email_key") != std::string::npos)
        {
            response["error"] =
                "Email already registered";
        }
        else
        {
            response["error"] =
                "Database error";
        }

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(response);

        if (error.find("users_email_key") != std::string::npos)
        {
            resp->setStatusCode(drogon::k409Conflict);
        }
        else
        {
            resp->setStatusCode(
                drogon::k500InternalServerError);
        }

        callback(resp);
    }
}

void AuthController::login(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        Json::Value response;

        if (!initializeSodium())
        {
            response["success"] = false;
            response["error"] = "Password security initialization failed";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            callback(resp);
            return;
        }

        auto json = request->getJsonObject();

        if (!json)
        {
            response["success"] = false;
            response["error"] = "Invalid JSON request";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        const std::string email =
            (*json)["email"].asString();

        const std::string password =
            (*json)["password"].asString();

        if (email.empty() || password.empty())
        {
            response["success"] = false;
            response["error"] =
                "Email and password are required";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "SELECT id, name, email, password_hash, role "
            "FROM users "
            "WHERE email = $1",
            email);

        if (result.empty())
        {
            response["success"] = false;
            response["error"] =
                "Invalid email or password";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        const auto& row = result[0];

        const std::string storedPasswordHash =
            row["password_hash"].as<std::string>();

        
        if (!verifyPassword(
                storedPasswordHash,
                password))
        {
            response["success"] = false;
            response["error"] =
                "Invalid email or password";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        const int userId =
            row["id"].as<int>();

        const std::string role =
            row["role"].as<std::string>();

        const std::string token =
            JwtService::generateToken(
                userId,
                role);

        response["success"] = true;
        response["message"] =
            "Login successful";

        response["data"]["token"] =
            token;

        response["data"]["user"]["id"] =
            userId;

        response["data"]["user"]["name"] =
            row["name"].as<std::string>();

        response["data"]["user"]["email"] =
            row["email"].as<std::string>();

        response["data"]["user"]["role"] =
            role;

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(response);

        resp->setStatusCode(drogon::k200OK);

        callback(resp);
    }
    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;
        response["error"] = exception.what();

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(response);

        resp->setStatusCode(
            drogon::k500InternalServerError);

        callback(resp);
    }
}

void AuthController::options(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();
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
    response->setStatusCode(drogon::k200OK);
    callback(response);
}
} // namespace kani::kanimart
