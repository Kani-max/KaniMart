#include "AuthController.h"

#include <drogon/drogon.h>
#include <json/json.h>

namespace kani::kanimart {

void AuthController::login(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        auto json = request->getJsonObject();

        Json::Value response;

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

        const std::string storedPassword =
            row["password_hash"].as<std::string>();

        if (storedPassword != password)
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

        response["success"] = true;
        response["message"] = "Login successful";

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

        resp->setStatusCode(drogon::k200OK);

        callback(resp);
    }
    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;
        response["error"] = "Database error";

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(response);

        resp->setStatusCode(
            drogon::k500InternalServerError);

        callback(resp);
    }
}


void AuthController::registerUser(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        auto json = request->getJsonObject();

        Json::Value response;

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

        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "INSERT INTO users "
            "(name, email, password_hash, role) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id, name, email, role",
            name,
            email,
            password,
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

} // namespace kani::kanimart