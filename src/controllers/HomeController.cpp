#include "HomeController.h"

void HomeController::home(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    Json::Value response;

    response["success"] = true;
    response["message"] = "Welcome to KaniMart";
    response["version"] = "1.0";
    response["status"] = "running";

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void HomeController::health(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    Json::Value response;

    response["success"] = true;
    response["service"] = "KaniMart Backend";
    response["status"] = "healthy";

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}