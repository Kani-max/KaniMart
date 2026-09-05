#pragma once

#include <drogon/HttpController.h>

namespace kani::kanimart {

class AuthController final
    : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        AuthController::registerUser,
        "/api/auth/register",
        drogon::Post);

    ADD_METHOD_TO(
        AuthController::login,
        "/api/auth/login",
        drogon::Post);

    METHOD_LIST_END

    void registerUser(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void login(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace kani::kanimart