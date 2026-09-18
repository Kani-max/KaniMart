#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "JwtService.h"

#include <functional>
#include <string>

namespace kani::kanimart
{

class AuthMiddleware final
{
public:
    struct User
    {
        int id = 0;
        std::string role;
    };

    static bool authenticate(
        const drogon::HttpRequestPtr& request,
        User& user)
    {
        const std::string authorization =
            request->getHeader("Authorization");

        constexpr const char* prefix = "Bearer ";

        if (authorization.size() <= 7 ||
            authorization.compare(
                0,
                7,
                prefix) != 0)
        {
            return false;
        }

        const std::string token =
            authorization.substr(7);

        if (token.empty())
        {
            return false;
        }

        return JwtService::verifyToken(
            token,
            user.id,
            user.role);
    }

    static drogon::HttpResponsePtr unauthorized()
    {
        Json::Value response;
        response["success"] = false;
        response["error"] =
            "Authentication required";

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        resp->setStatusCode(
            drogon::k401Unauthorized);

        return resp;
    }

    static drogon::HttpResponsePtr forbidden()
    {
        Json::Value response;
        response["success"] = false;
        response["error"] =
            "Access denied";

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        resp->setStatusCode(
            drogon::k403Forbidden);

        return resp;
    }
};

} // namespace kani::kanimart