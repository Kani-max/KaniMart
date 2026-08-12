#include "UserController.h"

void UserController::login(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    Json::Value response;

    if (!json)
    {
        response["success"] = false;
        response["message"] = "Invalid JSON request";

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    std::string email =
        (*json)["email"].asString();

    std::string password =
        (*json)["password"].asString();

    if (email.empty() || password.empty())
    {
        response["success"] = false;
        response["message"] = "Email and password are required";

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    /*
        Temporary prototype login.

        Database authentication will be connected
        after the API is confirmed working.
    */

    if (email == "admin@kanimart.com" &&
        password == "123456")
    {
        response["success"] = true;
        response["message"] = "Login successful";
        response["user"]["name"] = "KaniMart Admin";
        response["user"]["email"] = email;
    }
    else
    {
        response["success"] = false;
        response["message"] = "Invalid email or password";
    }

    auto resp =
        HttpResponse::newHttpJsonResponse(response);

    callback(resp);
}


void UserController::registerUser(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    Json::Value response;

    if (!json)
    {
        response["success"] = false;
        response["message"] = "Invalid JSON request";

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    std::string name =
        (*json)["name"].asString();

    std::string email =
        (*json)["email"].asString();

    std::string password =
        (*json)["password"].asString();

    if (name.empty() ||
        email.empty() ||
        password.empty())
    {
        response["success"] = false;
        response["message"] =
            "Name, email and password are required";

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k400BadRequest);

        callback(resp);
        return;
    }

    response["success"] = true;
    response["message"] = "Registration successful";
    response["user"]["name"] = name;
    response["user"]["email"] = email;

    auto resp =
        HttpResponse::newHttpJsonResponse(response);

    callback(resp);
}