#include <drogon/HttpController.h>

namespace kani::kanimart
{

class OrderController final
    : public drogon::HttpController<OrderController>
{
public:

    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        OrderController::checkout,
        "/api/orders/checkout/{userId}",
        drogon::Post);

    ADD_METHOD_TO(
        OrderController::checkoutOptions,
        "/api/orders/checkout/{userId}",
        drogon::Options);

    ADD_METHOD_TO(
        OrderController::getOrders,
        "/api/orders/{userId}",
        drogon::Get);

    ADD_METHOD_TO(
        OrderController::getOrdersOptions,
        "/api/orders/{userId}",
        drogon::Options);

    ADD_METHOD_TO(
        OrderController::getOrder,
        "/api/orders/{userId}/{orderId}",
        drogon::Get);

    ADD_METHOD_TO(
        OrderController::getOrderOptions,
        "/api/orders/{userId}/{orderId}",
        drogon::Options);

    ADD_METHOD_TO(
        OrderController::updateStatus,
        "/api/orders/{userId}/{orderId}/status",
        drogon::Put);

    ADD_METHOD_TO(
        OrderController::updateStatusOptions,
        "/api/orders/{userId}/{orderId}/status",
        drogon::Options);

    METHOD_LIST_END

    void checkout(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId);

    void checkoutOptions(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getOrders(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId);

    void getOrdersOptions(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getOrder(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId,
        int orderId);

    void getOrderOptions(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void updateStatus(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId,
        int orderId);

    void updateStatusOptions(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

}  // namespace kani::kanimart