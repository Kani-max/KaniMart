#pragma once
#include <drogon/HttpController.h>
namespace kani::kanimart {
class OrderController final
    : public drogon::HttpController<OrderController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        OrderController::checkout,
        "/api/orders/checkout/{1}",
        drogon::Post);
    ADD_METHOD_TO(
        OrderController::getOrders,
        "/api/orders/{1}",
        drogon::Get);
    ADD_METHOD_TO(
        OrderController::getOrder,
        "/api/orders/{1}/{2}",
        drogon::Get);
    METHOD_LIST_END
    void checkout(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId);
    void getOrders(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId);
    void getOrder(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId,
        int orderId);
};
}  // namespace kani::kanimart
