#pragma once

#include <drogon/HttpController.h>

namespace kani::kanimart {

class ProductController final
    : public drogon::HttpController<ProductController> {
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        ProductController::listProducts,
        "/api/products",
        drogon::Get);

    ADD_METHOD_TO(
        ProductController::createProduct,
        "/api/products",
        drogon::Post);

    ADD_METHOD_TO(
        ProductController::getProduct,
        "/api/products/{1}",
        drogon::Get);

    METHOD_LIST_END

    void listProducts(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void createProduct(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getProduct(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int productId);
};

}  // namespace kani::kanimart