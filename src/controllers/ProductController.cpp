#include "ProductController.h"

#include <drogon/drogon.h>

#include "../services/AuthMiddleware.h"

namespace kani::kanimart
{

namespace
{

bool authenticateUser(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>& callback,
    AuthMiddleware::User& user)
{
    if (!AuthMiddleware::authenticate(request, user))
    {
        callback(AuthMiddleware::unauthorized());
        return false;
    }

    return true;
}

bool requireSeller(
    const AuthMiddleware::User& user,
    std::function<void(const drogon::HttpResponsePtr&)>& callback)
{
    if (user.role != "SELLER" &&
        user.role != "ADMIN")
    {
        callback(AuthMiddleware::forbidden());
        return false;
    }

    return true;
}

bool authorizeProductOwner(
    int productSellerId,
    const AuthMiddleware::User& user,
    std::function<void(const drogon::HttpResponsePtr&)>& callback)
{
    if (user.role == "ADMIN")
    {
        return true;
    }

    if (productSellerId != user.id)
    {
        callback(AuthMiddleware::forbidden());
        return false;
    }

    return true;
}

} // namespace


void ProductController::listProducts(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "SELECT id, seller_id, name, description, "
            "price_cents, stock_qty, category, image_url, created_at "
            "FROM products "
            "WHERE id NOT IN (1, 3, 6, 8, 9) "
            "ORDER BY id DESC");

        Json::Value products(Json::arrayValue);

        for (const auto& row : result)
        {
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

            if (row["image_url"].isNull())
            {
                product["image_url"] =
                    Json::nullValue;
            }
            else
            {
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
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        callback(httpResponse);
    }
    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;
        response["error"] = exception.what();

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}


void ProductController::createProduct(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        AuthMiddleware::User user;

        if (!authenticateUser(
                request,
                callback,
                user))
        {
            return;
        }

        if (!requireSeller(
                user,
                callback))
        {
            return;
        }

        if (!request->getJsonObject())
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "Request body must be valid JSON";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k400BadRequest);

            callback(httpResponse);
            return;
        }

        const auto& body =
            *request->getJsonObject();

        /*
         * seller_id is intentionally NOT taken from
         * the request body.
         *
         * The authenticated JWT determines the seller.
         */
        const int sellerId =
            user.id;

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

        std::string imageUrl;

        if (body.isMember("image_url") &&
            !body["image_url"].isNull())
        {
            imageUrl =
                body["image_url"].asString();
        }

        if (name.empty() ||
            description.empty() ||
            priceCents < 0 ||
            stockQty < 0 ||
            category.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "Invalid product data";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

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
            "stock_qty, category, image_url) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7)",
            sellerId,
            name,
            description,
            priceCents,
            stockQty,
            category,
            imageUrl);

        Json::Value response;

        response["success"] = true;
        response["message"] =
            "Product created";

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        httpResponse->setStatusCode(
            drogon::k201Created);

        callback(httpResponse);
    }
    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;
        response["error"] = exception.what();

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}


void ProductController::getProduct(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId)
{
    try
    {
        if (productId <= 0)
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "Invalid product ID";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

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

        if (result.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "Product not found";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k404NotFound);

            callback(httpResponse);
            return;
        }

        const auto& row =
            result[0];

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

        if (row["image_url"].isNull())
        {
            product["image_url"] =
                Json::nullValue;
        }
        else
        {
            product["image_url"] =
                row["image_url"].as<std::string>();
        }

        product["created_at"] =
            row["created_at"].as<std::string>();

        Json::Value response;

        response["success"] = true;
        response["data"] = product;

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        httpResponse->setStatusCode(
            drogon::k200OK);

        callback(httpResponse);
    }
    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;
        response["error"] =
            exception.what();

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}


void ProductController::updateProduct(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(
            request,
            callback,
            user))
    {
        return;
    }

    if (!requireSeller(
            user,
            callback))
    {
        return;
    }

    if (productId <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Invalid product ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    auto json =
        request->getJsonObject();

    if (!json)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Request body must be valid JSON";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    if (!json->isMember("name") ||
        !(*json)["name"].isString() ||
        (*json)["name"].asString().empty())
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Name is required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    if (!json->isMember("description") ||
        !(*json)["description"].isString())
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Description is required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    if (!json->isMember("price_cents") ||
        !(*json)["price_cents"].isInt() ||
        (*json)["price_cents"].asInt() < 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "price_cents must be a non-negative integer";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    if (!json->isMember("stock_qty") ||
        !(*json)["stock_qty"].isInt() ||
        (*json)["stock_qty"].asInt() < 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "stock_qty must be a non-negative integer";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    if (!json->isMember("category") ||
        !(*json)["category"].isString() ||
        (*json)["category"].asString().empty())
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Category is required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");

        const auto ownerResult =
            db->execSqlSync(
                "SELECT seller_id "
                "FROM products "
                "WHERE id = $1",
                productId);

        if (ownerResult.empty())
        {
            Json::Value body;

            body["success"] = false;
            body["message"] =
                "Product not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    body);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        const int productSellerId =
            ownerResult[0]["seller_id"].as<int>();

        if (!authorizeProductOwner(
                productSellerId,
                user,
                callback))
        {
            return;
        }

        const auto name =
            (*json)["name"].asString();

        const auto description =
            (*json)["description"].asString();

        const int priceCents =
            (*json)["price_cents"].asInt();

        const int stockQty =
            (*json)["stock_qty"].asInt();

        const auto category =
            (*json)["category"].asString();

        std::string imageUrl;

        if (json->isMember("image_url") &&
            !(*json)["image_url"].isNull())
        {
            if (!(*json)["image_url"].isString())
            {
                Json::Value body;

                body["success"] = false;
                body["message"] =
                    "image_url must be a string or null";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        body);

                response->setStatusCode(
                    drogon::k400BadRequest);

                callback(response);
                return;
            }

            imageUrl =
                (*json)["image_url"].asString();
        }

        drogon::orm::Result result;

        if (imageUrl.empty())
        {
            result = db->execSqlSync(
                "UPDATE products "
                "SET name = $1, description = $2, "
                "price_cents = $3, stock_qty = $4, "
                "category = $5, image_url = NULL "
                "WHERE id = $6 "
                "RETURNING id, seller_id, name, description, "
                "price_cents, stock_qty, category, image_url, "
                "created_at",
                name,
                description,
                priceCents,
                stockQty,
                category,
                productId);
        }
        else
        {
            result = db->execSqlSync(
                "UPDATE products "
                "SET name = $1, description = $2, "
                "price_cents = $3, stock_qty = $4, "
                "category = $5, image_url = $6 "
                "WHERE id = $7 "
                "RETURNING id, seller_id, name, description, "
                "price_cents, stock_qty, category, image_url, "
                "created_at",
                name,
                description,
                priceCents,
                stockQty,
                category,
                imageUrl,
                productId);
        }

        const auto& row =
            result[0];

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

        if (row["image_url"].isNull())
        {
            product["image_url"] =
                Json::nullValue;
        }
        else
        {
            product["image_url"] =
                row["image_url"].as<std::string>();
        }

        product["created_at"] =
            row["created_at"].as<std::string>();

        Json::Value body;

        body["success"] = true;
        body["message"] =
            "Product updated successfully";
        body["data"] = product;

        callback(
            drogon::HttpResponse::newHttpJsonResponse(
                body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            std::string("Failed to update product: ") +
            e.what();

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k500InternalServerError);

        callback(response);
    }
}


void ProductController::deleteProduct(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(
            request,
            callback,
            user))
    {
        return;
    }

    if (!requireSeller(
            user,
            callback))
    {
        return;
    }

    if (productId <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Invalid product ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");

        const auto ownerResult =
            db->execSqlSync(
                "SELECT seller_id "
                "FROM products "
                "WHERE id = $1",
                productId);

        if (ownerResult.empty())
        {
            Json::Value body;

            body["success"] = false;
            body["message"] =
                "Product not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    body);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        const int productSellerId =
            ownerResult[0]["seller_id"].as<int>();

        if (!authorizeProductOwner(
                productSellerId,
                user,
                callback))
        {
            return;
        }

        auto result =
            db->execSqlSync(
                "DELETE FROM products "
                "WHERE id = $1 "
                "RETURNING id",
                productId);

        if (result.empty())
        {
            Json::Value body;

            body["success"] = false;
            body["message"] =
                "Product not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    body);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        Json::Value body;

        body["success"] = true;
        body["message"] =
            "Product deleted successfully";
        body["data"]["id"] =
            result[0]["id"].as<int>();

        callback(
            drogon::HttpResponse::newHttpJsonResponse(
                body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            std::string("Failed to delete product: ") +
            e.what();

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                body);

        response->setStatusCode(
            drogon::k500InternalServerError);

        callback(response);
    }
}


void ProductController::options(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response =
        drogon::HttpResponse::newHttpResponse();

    response->setStatusCode(
        drogon::k200OK);

    response->addHeader(
        "Access-Control-Allow-Origin",
        "http://127.0.0.1:5500");

    response->addHeader(
        "Access-Control-Allow-Methods",
        "GET, POST, PUT, DELETE, OPTIONS");

    response->addHeader(
        "Access-Control-Allow-Headers",
        "Content-Type, Authorization");

    response->addHeader(
        "Access-Control-Allow-Credentials",
        "true");

    callback(response);
}


void ProductController::optionsById(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int productId)
{
    options(request, std::move(callback));
}


} // namespace kani::kanimart