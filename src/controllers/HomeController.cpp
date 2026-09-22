#include "HomeController.h"

void HomeController::home(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    callback(HttpResponse::newRedirectionResponse("/index.html"));
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