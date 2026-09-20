#pragma once
#include <drogon/HttpController.h>
namespace kani::kanimart {
class CartController final
    : public drogon::HttpController<CartController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        CartController::getCart,
        "/api/cart/{id}",
        drogon::Get);
    ADD_METHOD_TO(
        CartController::addToCart,
        "/api/cart/{id}",
        drogon::Post);
    ADD_METHOD_TO(
        CartController::updateCartItem,
        "/api/cart/{id}/{product_id}",
        drogon::Put);
    ADD_METHOD_TO(
        CartController::removeFromCart,
        "/api/cart/{id}/{product_id}",
        drogon::Delete);
        
    ADD_METHOD_TO(
        CartController::options, 
        "/api/cart/{id}", 
        drogon::Options);
    ADD_METHOD_TO(
        CartController::options, 
        "/api/cart/{id}/{product_id}", 
        drogon::Options);
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
    void options(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
}  // namespace kani::kanimart
