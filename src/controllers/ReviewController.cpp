#include "ReviewController.h"
#include <drogon/drogon.h>
namespace kani::kanimart
{
void ReviewController::createReview(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId,
    int userId)
{
    try
    {
        if (productId <= 0 || userId <= 0)
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "Invalid product or user ID";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        auto json = request->getJsonObject();
        if (!json)
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "Request body must be valid JSON";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        if (!json->isMember("rating") ||
            !(*json)["rating"].isInt())
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "rating is required and must be an integer";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        if (!json->isMember("comment") ||
            !(*json)["comment"].isString())
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "comment is required and must be a string";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        const int rating = (*json)["rating"].asInt();
        const std::string comment = (*json)["comment"].asString();
        if (rating < 1 || rating > 5)
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "rating must be between 1 and 5";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        if (comment.empty())
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "comment cannot be empty";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        auto db = drogon::app().getDbClient("kanimart");
        const auto productRows = db->execSqlSync(
            "SELECT id FROM products WHERE id = $1",
            productId);
        if (productRows.empty())
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "Product not found";
            callback(
                drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        const auto userRows = db->execSqlSync(
            "SELECT id FROM users WHERE id = $1",
            userId);
        if (userRows.empty())
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "User not found";
            callback(
                drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        const auto rows = db->execSqlSync(
            "INSERT INTO reviews "
            "(product_id, user_id, rating, comment) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id, product_id, user_id, rating, "
            "comment, created_at",
            productId,
            userId,
            rating,
            comment);
        Json::Value review;
        review["id"] = rows[0]["id"].as<int>();
        review["product_id"] =
            rows[0]["product_id"].as<int>();
        review["user_id"] =
            rows[0]["user_id"].as<int>();
        review["rating"] =
            rows[0]["rating"].as<int>();
        review["comment"] =
            rows[0]["comment"].as<std::string>();
        review["created_at"] =
            rows[0]["created_at"].as<std::string>();
        Json::Value body;
        body["success"] = true;
        body["message"] = "Review created";
        body["data"] = review;
        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;
        body["success"] = false;
        body["message"] =
            std::string("Failed to create review: ") + e.what();
        callback(
            drogon::HttpResponse::newHttpJsonResponse(body));
    }
}
void ReviewController::getReviews(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId)
{
    try
    {
        if (productId <= 0)
        {
            Json::Value body;
            body["success"] = false;
            body["message"] = "Invalid product ID";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
            return;
        }
        auto db = drogon::app().getDbClient("kanimart");
        const auto rows = db->execSqlSync(
            "SELECT r.id, r.product_id, r.user_id, "
            "u.name AS user_name, r.rating, r.comment, "
            "r.created_at "
            "FROM reviews r "
            "JOIN users u ON u.id = r.user_id "
            "WHERE r.product_id = $1 "
            "ORDER BY r.created_at DESC, r.id DESC",
            productId);
        Json::Value reviews(Json::arrayValue);
        for (const auto& row : rows)
        {
            Json::Value review;
            review["id"] = row["id"].as<int>();
            review["product_id"] =
                row["product_id"].as<int>();
            review["user_id"] =
                row["user_id"].as<int>();
            review["user_name"] =
                row["user_name"].as<std::string>();
            review["rating"] =
                row["rating"].as<int>();
            review["comment"] =
                row["comment"].as<std::string>();
            review["created_at"] =
                row["created_at"].as<std::string>();
            reviews.append(review);
        }
        Json::Value data;
        data["reviews"] = reviews;
        Json::Value body;
        body["success"] = true;
        body["data"] = data;
        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;
        body["success"] = false;
        body["message"] =
            std::string("Failed to load reviews: ") + e.what();
        callback(
            drogon::HttpResponse::newHttpJsonResponse(body));
    }
}
} // namespace kani::kanimart


