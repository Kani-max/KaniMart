#include "AdminController.h"
#include <drogon/drogon.h>
#include "../services/AuthMiddleware.h"
namespace kani::kanimart
{
namespace
{
bool requireAdmin(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>& callback)
{
    AuthMiddleware::User user;
    if (!AuthMiddleware::authenticate(request, user))
    {
        callback(AuthMiddleware::unauthorized());
        return false;
    }
    if (user.role != "ADMIN")
    {
        callback(AuthMiddleware::forbidden());
        return false;
    }
    return true;
}
} // namespace
void AdminController::listUsers(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!requireAdmin(request, callback))
    {
        return;
    }
    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");
        auto rows =
            db->execSqlSync(
                "SELECT id, name, email, role, created_at "
                "FROM users "
                "ORDER BY id DESC");
        Json::Value users(Json::arrayValue);
        for (const auto& row : rows)
        {
            Json::Value user;
            user["id"] =
                row["id"].as<int>();
            user["name"] =
                row["name"].as<std::string>();
            user["email"] =
                row["email"].as<std::string>();
            user["role"] =
                row["role"].as<std::string>();
            user["created_at"] =
                row["created_at"].as<std::string>();
            users.append(user);
        }
        Json::Value data;
        data["users"] = users;
        Json::Value body;
        body["success"] = true;
        body["data"] = data;
        callback(
            drogon::HttpResponse::newHttpJsonResponse(body));
    }
    catch (const std::exception&)
    {
        Json::Value body;
        body["success"] = false;
        body["message"] = "Failed to load users";
        auto response =
            drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(
            drogon::k500InternalServerError);
        callback(response);
    }
}
void AdminController::listOrders(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    if (!requireAdmin(request, callback))
    {
        return;
    }
    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");
        auto rows =
            db->execSqlSync(
                "SELECT o.id, o.buyer_id, u.name AS buyer_name, "
                "u.email AS buyer_email, o.status, "
                "o.total_amount_cents, o.created_at "
                "FROM orders o "
                "JOIN users u ON u.id = o.buyer_id "
                "ORDER BY o.created_at DESC, o.id DESC");
        Json::Value orders(Json::arrayValue);
        for (const auto& row : rows)
        {
            Json::Value order;
            order["id"] =
                row["id"].as<int>();
            order["buyer_id"] =
                row["buyer_id"].as<int>();
            order["buyer_name"] =
                row["buyer_name"].as<std::string>();
            order["buyer_email"] =
                row["buyer_email"].as<std::string>();
            order["status"] =
                row["status"].as<std::string>();
            order["total_amount_cents"] =
                static_cast<Json::Int64>(
                    row["total_amount_cents"].as<long long>());
            order["created_at"] =
                row["created_at"].as<std::string>();
            orders.append(order);
        }
        Json::Value data;
        data["orders"] = orders;
        Json::Value body;
        body["success"] = true;
        body["data"] = data;
        callback(
            drogon::HttpResponse::newHttpJsonResponse(body));
    }
    catch (const std::exception&)
    {
        Json::Value body;
        body["success"] = false;
        body["message"] = "Failed to load orders";
        auto response =
            drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(
            drogon::k500InternalServerError);
        callback(response);
    }
}
} // namespace kani::kanimart
