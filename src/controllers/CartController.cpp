#include "CartController.h"

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

bool authorizeUser(
    int requestedUserId,
    const AuthMiddleware::User& user,
    std::function<void(const drogon::HttpResponsePtr&)>& callback)
{
    if (requestedUserId != user.id)
    {
        callback(AuthMiddleware::forbidden());
        return false;
    }

    return true;
}

} // namespace

void CartController::getCart(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int userId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(request, callback, user))
    {
        return;
    }

    if (!authorizeUser(userId, user, callback))
    {
        return;
    }

    Json::Value json;

    if (userId <= 0)
    {
        json["success"] = false;
        json["error"] = "Invalid user ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "SELECT ci.product_id, p.name, p.price_cents, "
            "       ci.quantity, "
            "       (p.price_cents * ci.quantity) AS subtotal_cents "
            "FROM cart_items ci "
            "JOIN products p ON p.id = ci.product_id "
            "WHERE ci.user_id = $1 "
            "ORDER BY ci.id",
            userId);

        Json::Value items(Json::arrayValue);
        long long totalCents = 0;

        for (const auto& row : result)
        {
            Json::Value item;

            item["product_id"] =
                row["product_id"].as<int>();

            item["name"] =
                row["name"].as<std::string>();

            item["price_cents"] =
                row["price_cents"].as<int>();

            item["quantity"] =
                row["quantity"].as<int>();

            const auto subtotal =
                row["subtotal_cents"].as<long long>();

                item["subtotal_cents"] = static_cast<Json::Int64>(subtotal);
                totalCents += subtotal;

            items.append(item);
        }

        json["success"] = true;
        json["data"]["items"] = items;
        json["data"]["total_cents"] = static_cast<Json::Int64>(totalCents);
        
        callback(
            drogon::HttpResponse::newHttpJsonResponse(json));
    }
    catch (const std::exception& e)
    {
        json["success"] = false;
        json["error"] = "Failed to retrieve cart";

        LOG_ERROR << "getCart error: " << e.what();

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(
            drogon::k500InternalServerError);

        callback(response);
    }
}

void CartController::addToCart(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int userId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(request, callback, user))
    {
        return;
    }

    if (!authorizeUser(userId, user, callback))
    {
        return;
    }

    Json::Value json;

    if (userId <= 0)
    {
        json["success"] = false;
        json["error"] = "Invalid user ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    try
    {
        const auto body =
            request->getJsonObject();

        if (!body ||
            !body->isMember("product_id") ||
            !body->isMember("quantity"))
        {
            json["success"] = false;
            json["error"] =
                "product_id and quantity are required";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k400BadRequest);

            callback(response);
            return;
        }

        const int productId =
            (*body)["product_id"].asInt();

        const int quantity =
            (*body)["quantity"].asInt();

        if (productId <= 0 || quantity <= 0)
        {
            json["success"] = false;
            json["error"] =
                "product_id and quantity must be positive";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k400BadRequest);

            callback(response);
            return;
        }

        auto db =
            drogon::app().getDbClient("kanimart");

        auto product = db->execSqlSync(
            "SELECT id, stock_qty "
            "FROM products "
            "WHERE id = $1",
            productId);

        if (product.empty())
        {
            json["success"] = false;
            json["error"] = "Product not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        const int stockQty =
            product[0]["stock_qty"].as<int>();

        auto existing = db->execSqlSync(
            "SELECT quantity "
            "FROM cart_items "
            "WHERE user_id = $1 "
            "AND product_id = $2",
            userId,
            productId);

        const int existingQty =
            existing.empty()
                ? 0
                : existing[0]["quantity"].as<int>();

        if (existingQty + quantity > stockQty)
        {
            json["success"] = false;
            json["error"] =
                "Requested quantity exceeds available stock";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k400BadRequest);

            callback(response);
            return;
        }

        db->execSqlSync(
            "INSERT INTO cart_items "
            "(user_id, product_id, quantity) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (user_id, product_id) "
            "DO UPDATE SET "
            "quantity = cart_items.quantity + EXCLUDED.quantity",
            userId,
            productId,
            quantity);

        json["success"] = true;
        json["message"] = "Product added to cart";

        callback(
            drogon::HttpResponse::newHttpJsonResponse(json));
    }
    catch (const std::exception& e)
    {
        json["success"] = false;
        json["error"] =
            "Failed to add product to cart";

        LOG_ERROR << "addToCart error: "
                  << e.what();

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(
            drogon::k500InternalServerError);

        callback(response);
    }
}

void CartController::updateCartItem(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int userId,
    int productId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(request, callback, user))
    {
        return;
    }

    if (!authorizeUser(userId, user, callback))
    {
        return;
    }

    Json::Value json;

    if (userId <= 0 || productId <= 0)
    {
        json["success"] = false;
        json["error"] =
            "Invalid user ID or product ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    try
    {
        const auto body =
            request->getJsonObject();

        if (!body ||
            !body->isMember("quantity"))
        {
            json["success"] = false;
            json["error"] =
                "quantity is required";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k400BadRequest);

            callback(response);
            return;
        }

        const int quantity =
            (*body)["quantity"].asInt();

        if (quantity <= 0)
        {
            json["success"] = false;
            json["error"] =
                "quantity must be positive";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k400BadRequest);

            callback(response);
            return;
        }

        auto db =
            drogon::app().getDbClient("kanimart");

        auto product = db->execSqlSync(
            "SELECT stock_qty "
            "FROM products "
            "WHERE id = $1",
            productId);

        if (product.empty())
        {
            json["success"] = false;
            json["error"] =
                "Product not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        const int stockQty =
            product[0]["stock_qty"].as<int>();

        if (quantity > stockQty)
        {
            json["success"] = false;
            json["error"] =
                "Requested quantity exceeds available stock";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k400BadRequest);

            callback(response);
            return;
        }

        auto result = db->execSqlSync(
            "UPDATE cart_items "
            "SET quantity = $1 "
            "WHERE user_id = $2 "
            "AND product_id = $3",
            quantity,
            userId,
            productId);

        if (result.affectedRows() == 0)
        {
            json["success"] = false;
            json["error"] =
                "Cart item not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        json["success"] = true;
        json["message"] =
            "Cart item updated";

        callback(
            drogon::HttpResponse::newHttpJsonResponse(json));
    }
    catch (const std::exception& e)
    {
        json["success"] = false;
        json["error"] =
            "Failed to update cart item";

        LOG_ERROR << "updateCartItem error: "
                  << e.what();

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(
            drogon::k500InternalServerError);

        callback(response);
    }
}

void CartController::removeFromCart(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int userId,
    int productId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(request, callback, user))
    {
        return;
    }

    if (!authorizeUser(userId, user, callback))
    {
        return;
    }

    Json::Value json;

    if (userId <= 0 || productId <= 0)
    {
        json["success"] = false;
        json["error"] =
            "Invalid user ID or product ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(
            drogon::k400BadRequest);

        callback(response);
        return;
    }

    try
    {
        auto db =
            drogon::app().getDbClient("kanimart");

        auto result = db->execSqlSync(
            "DELETE FROM cart_items "
            "WHERE user_id = $1 "
            "AND product_id = $2",
            userId,
            productId);

        if (result.affectedRows() == 0)
        {
            json["success"] = false;
            json["error"] =
                "Cart item not found";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(json);

            response->setStatusCode(
                drogon::k404NotFound);

            callback(response);
            return;
        }

        json["success"] = true;
        json["message"] =
            "Product removed from cart";

        callback(
            drogon::HttpResponse::newHttpJsonResponse(json));
    }
    catch (const std::exception& e)
    {
        json["success"] = false;
        json["error"] =
            "Failed to remove cart item";

        LOG_ERROR << "removeFromCart error: "
                  << e.what();

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(json);

        response->setStatusCode(
            drogon::k500InternalServerError);

        callback(response);
    }
}
void CartController::options(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();

    const auto origin = request->getHeader("Origin");

    if (origin == "http://127.0.0.1:5500" ||
        origin == "http://localhost:5500")
    {
        response->addHeader(
            "Access-Control-Allow-Origin",
            origin);

        response->addHeader(
            "Access-Control-Allow-Methods",
            "GET, POST, PUT, DELETE, OPTIONS");

        response->addHeader(
            "Access-Control-Allow-Headers",
            "Content-Type, Authorization");

        response->addHeader(
            "Access-Control-Allow-Credentials",
            "true");
    }

    response->setStatusCode(drogon::k200OK);
    callback(response);
}

} // namespace kani::kanimart