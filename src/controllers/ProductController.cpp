#include "ProductController.h"

#include <drogon/drogon.h>

namespace kani::kanimart {

void ProductController::listProducts(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

    try {
        auto db = drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "SELECT id, seller_id, name, description, "
            "price_cents, stock_qty, category, image_url, created_at "
            "FROM products "
            "ORDER BY id DESC");

        Json::Value products(Json::arrayValue);

        for (const auto& row : result) {
            Json::Value product;

            product["id"] = row["id"].as<int>();
            product["seller_id"] = row["seller_id"].as<int>();
            product["name"] = row["name"].as<std::string>();
            product["description"] =
                row["description"].as<std::string>();
            product["price_cents"] =
                row["price_cents"].as<int>();
            product["stock_qty"] =
                row["stock_qty"].as<int>();
            product["category"] =
                row["category"].as<std::string>();

            if (row["image_url"].isNull()) {
                product["image_url"] = Json::nullValue;
            } else {
                product["image_url"] =
                    row["image_url"].as<std::string>();
            }

            product["created_at"] =
                row["created_at"].as<std::string>();

            products.append(product);
        }

        Json::Value response;
        response["success"] = true;
        response["data"] = products;

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(response);

        callback(httpResponse);
    }
    catch (const std::exception& exception) {
        Json::Value response;
        response["success"] = false;
        response["error"] = exception.what();

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}


void ProductController::createProduct(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

    try {
        if (!request->getJsonObject()) {
            Json::Value response;
            response["success"] = false;
            response["error"] =
                "Request body must be valid JSON";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                drogon::k400BadRequest);

            callback(httpResponse);
            return;
        }

        const auto& body = *request->getJsonObject();

        const int sellerId =
            body["seller_id"].asInt();

        const std::string name =
            body["name"].asString();

        const std::string description =
            body["description"].asString();

        const int priceCents =
            body["price_cents"].asInt();

        const int stockQty =
            body["stock_qty"].asInt();

        const std::string category =
            body["category"].asString();

        if (sellerId <= 0 ||
            name.empty() ||
            description.empty() ||
            priceCents < 0 ||
            stockQty < 0 ||
            category.empty()) {

            Json::Value response;
            response["success"] = false;
            response["error"] =
                "Invalid product data";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                drogon::k400BadRequest);

            callback(httpResponse);
            return;
        }

        auto db =
            drogon::app().getDbClient("kanimart");

        db->execSqlSync(
            "INSERT INTO products "
            "(seller_id, name, description, price_cents, "
            "stock_qty, category) "
            "VALUES ($1, $2, $3, $4, $5, $6)",
            sellerId,
            name,
            description,
            priceCents,
            stockQty,
            category);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Product created";

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(
            drogon::k201Created);

        callback(httpResponse);
    }
    catch (const std::exception& exception) {
        Json::Value response;
        response["success"] = false;
        response["error"] = exception.what();

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}


void ProductController::getProduct(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId) {

    try {
        if (productId <= 0) {
            Json::Value response;
            response["success"] = false;
            response["error"] = "Invalid product ID";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                drogon::k400BadRequest);

            callback(httpResponse);
            return;
        }

        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "SELECT id, seller_id, name, description, "
            "price_cents, stock_qty, category, image_url, created_at "
            "FROM products "
            "WHERE id = $1",
            productId);

        if (result.empty()) {
            Json::Value response;
            response["success"] = false;
            response["error"] = "Product not found";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                drogon::k404NotFound);

            callback(httpResponse);
            return;
        }

        const auto& row = result[0];

        Json::Value product;

        product["id"] =
            row["id"].as<int>();

        product["seller_id"] =
            row["seller_id"].as<int>();

        product["name"] =
            row["name"].as<std::string>();

        product["description"] =
            row["description"].as<std::string>();

        product["price_cents"] =
            row["price_cents"].as<int>();

        product["stock_qty"] =
            row["stock_qty"].as<int>();

        product["category"] =
            row["category"].as<std::string>();

        if (row["image_url"].isNull()) {
            product["image_url"] =
                Json::nullValue;
        } else {
            product["image_url"] =
                row["image_url"].as<std::string>();
        }

        product["created_at"] =
            row["created_at"].as<std::string>();

        Json::Value response;
        response["success"] = true;
        response["data"] = product;

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(
            drogon::k200OK);

        callback(httpResponse);
    }
    catch (const std::exception& exception) {
        Json::Value response;
        response["success"] = false;
        response["error"] = exception.what();

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}

}  // namespace kani::kanimart
