#pragma once

#include <drogon/HttpController.h>

namespace kani::kanimart
{

class ReviewController final
    : public drogon::HttpController<ReviewController>
{
public:

    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        ReviewController::createReview,
        "/api/products/{1}/reviews/{2}",
        drogon::Post);

    ADD_METHOD_TO(
        ReviewController::getReviews,
        "/api/products/{1}/reviews",
        drogon::Get);

    METHOD_LIST_END

    void createReview(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int productId,
        int userId);

    void getReviews(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int productId);
};

} // namespace kani::kanimart