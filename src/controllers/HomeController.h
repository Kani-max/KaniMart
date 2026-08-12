#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class HomeController : public HttpController<HomeController>
{
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(HomeController::home, "/", Get);
    ADD_METHOD_TO(HomeController::health, "/api/health", Get);

    METHOD_LIST_END

    void home(const HttpRequestPtr& req,
              std::function<void(const HttpResponsePtr&)>&& callback);

    void health(const HttpRequestPtr& req,
                std::function<void(const HttpResponsePtr&)>&& callback);
};