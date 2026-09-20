#pragma once
#include <drogon/HttpController.h>
namespace kani::kanimart
{
class AdminController final
    : public drogon::HttpController<AdminController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        AdminController::listUsers,
        "/api/admin/users",
        drogon::Get);
    ADD_METHOD_TO(
        AdminController::listOrders,
        "/api/admin/orders",
        drogon::Get);
    METHOD_LIST_END
    void listUsers(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void listOrders(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
} // namespace kani::kanimart
