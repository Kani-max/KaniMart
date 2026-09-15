#pragma once
#include <drogon/HttpController.h>
namespace kani::kanimart {
class CartController final
    : public drogon::HttpController<CartController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        CartController::getCart,
        "/api/cart/{1}",
        drogon::Get);
    ADD_METHOD_TO(
        CartController::addToCart,
        "/api/cart/{1}",
        drogon::Post);
    ADD_METHOD_TO(
        CartController::updateCartItem,
        "/api/cart/{1}/{2}",
        drogon::Put);
    ADD_METHOD_TO(
        CartController::removeFromCart,
        "/api/cart/{1}/{2}",
        drogon::Delete);
    METHOD_LIST_END
    void getCart(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId);
    void addToCart(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId);
    void updateCartItem(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId,
        int productId);
    void removeFromCart(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId,
        int productId);
};
}  // namespace kani::kanimart
