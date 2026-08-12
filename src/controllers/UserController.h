#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class UserController : public HttpController<UserController>
{
public:

    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        UserController::login,
        "/api/login",
        Post
    );

    ADD_METHOD_TO(
        UserController::registerUser,
        "/api/register",
        Post
    );

    METHOD_LIST_END


    void login(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );


    void registerUser(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );
};