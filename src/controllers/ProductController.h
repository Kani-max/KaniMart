#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ProductController : public HttpController<ProductController>
{
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(ProductController::list, "/api/products", Get);

    METHOD_LIST_END

    void list(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
};