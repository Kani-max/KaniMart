#include "OrderController.h"

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

drogon::HttpResponsePtr jsonResponse(
    const Json::Value& body,
    drogon::HttpStatusCode status = drogon::k200OK)
{
    auto response =
        drogon::HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(status);

    return response;
}

} // namespace

void OrderController::checkout(
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

    if (userId <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] = "Invalid user ID";

        callback(
            jsonResponse(
                body,
                drogon::k400BadRequest));

        return;
    }

    auto db =
        drogon::app().getDbClient("kanimart");

    try
    {
        auto transaction =
            db->newTransaction();

        auto cartRows =
            transaction->execSqlSync(
                "SELECT c.product_id, c.quantity, "
                "p.name, p.price_cents, p.stock_qty "
                "FROM cart_items c "
                "JOIN products p ON p.id = c.product_id "
                "WHERE c.user_id = $1 "
                "ORDER BY c.product_id "
                "FOR UPDATE OF c, p",
                userId);

        if (cartRows.empty())
        {
            Json::Value body;

            body["success"] = false;
            body["message"] = "Cart is empty";

            callback(
                jsonResponse(
                    body,
                    drogon::k400BadRequest));

            return;
        }

        int totalCents = 0;

        for (const auto& row : cartRows)
        {
            const int quantity =
                row["quantity"].as<int>();

            const int stockQty =
                row["stock_qty"].as<int>();

            const int priceCents =
                row["price_cents"].as<int>();

            if (quantity <= 0)
            {
                Json::Value body;

                body["success"] = false;
                body["message"] =
                    "Invalid cart quantity";

                callback(
                    jsonResponse(
                        body,
                        drogon::k400BadRequest));

                return;
            }

            if (quantity > stockQty)
            {
                Json::Value body;

                body["success"] = false;
                body["message"] =
                    "Insufficient stock for product " +
                    std::to_string(
                        row["product_id"].as<int>());

                callback(
                    jsonResponse(
                        body,
                        drogon::k400BadRequest));

                return;
            }

            totalCents +=
                priceCents * quantity;
        }

        auto orderRows =
            transaction->execSqlSync(
                "INSERT INTO orders "
                "(buyer_id, status, total_amount_cents) "
                "VALUES ($1, 'PENDING', $2) "
                "RETURNING id, created_at",
                userId,
                totalCents);

        const int orderId =
            orderRows[0]["id"].as<int>();

        Json::Value items(
            Json::arrayValue);

        for (const auto& row : cartRows)
        {
            const int productId =
                row["product_id"].as<int>();

            const int quantity =
                row["quantity"].as<int>();

            const int priceCents =
                row["price_cents"].as<int>();

            transaction->execSqlSync(
                "INSERT INTO order_items "
                "(order_id, product_id, quantity, unit_price_cents) "
                "VALUES ($1, $2, $3, $4)",
                orderId,
                productId,
                quantity,
                priceCents);

            transaction->execSqlSync(
                "UPDATE products "
                "SET stock_qty = stock_qty - $1 "
                "WHERE id = $2",
                quantity,
                productId);

            Json::Value item;

            item["product_id"] =
                productId;

            item["quantity"] =
                quantity;

            item["unit_price_cents"] =
                static_cast<Json::Int64>(
                    priceCents);

            item["subtotal_cents"] =
                static_cast<Json::Int64>(
                    priceCents * quantity);

            items.append(item);
        }

        transaction->execSqlSync(
            "DELETE FROM cart_items "
            "WHERE user_id = $1",
            userId);

        Json::Value data;

        data["order_id"] =
            orderId;

        data["status"] =
            "PENDING";

        data["total_amount_cents"] =
            static_cast<Json::Int64>(
                totalCents);

        data["items"] =
            items;

        Json::Value body;

        body["success"] =
            true;

        body["message"] =
            "Checkout completed";

        body["data"] =
            data;

        callback(
            jsonResponse(body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;

        body["success"] =
            false;

        body["message"] =
            std::string(
                "Checkout failed: ") +
            e.what();

        callback(
            jsonResponse(
                body,
                drogon::k500InternalServerError));
    }
}

void OrderController::getOrders(
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

    if (userId <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Invalid user ID";

        callback(
            jsonResponse(
                body,
                drogon::k400BadRequest));

        return;
    }

    auto db =
        drogon::app().getDbClient("kanimart");

    try
    {
        auto rows =
            db->execSqlSync(
                "SELECT id, status, total_amount_cents, created_at "
                "FROM orders "
                "WHERE buyer_id = $1 "
                "ORDER BY created_at DESC",
                userId);

        Json::Value orders(
            Json::arrayValue);

        for (const auto& row : rows)
        {
            Json::Value order;

            order["id"] =
                row["id"].as<int>();

            order["status"] =
                row["status"].as<std::string>();

            order["total_amount_cents"] =
                static_cast<Json::Int64>(
                    row["total_amount_cents"].as<int>());

            order["created_at"] =
                row["created_at"].as<std::string>();

            orders.append(order);
        }

        Json::Value data;

        data["orders"] =
            orders;

        Json::Value body;

        body["success"] =
            true;

        body["data"] =
            data;

        callback(
            jsonResponse(body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;

        body["success"] =
            false;

        body["message"] =
            std::string(
                "Failed to load orders: ") +
            e.what();

        callback(
            jsonResponse(
                body,
                drogon::k500InternalServerError));
    }
}

void OrderController::getSellerOrders(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    AuthMiddleware::User user;

    if (!authenticateUser(request, callback, user))
    {
        return;
    }

    if (user.role != "SELLER" && user.role != "ADMIN")
    {
        callback(AuthMiddleware::forbidden());
        return;
    }

    auto db = drogon::app().getDbClient("kanimart");

    try
    {
        drogon::orm::Result rows;

        if (user.role == "ADMIN")
        {
            rows = db->execSqlSync(
                "SELECT o.id, o.status, o.total_amount_cents, "
                "o.created_at, o.buyer_id, u.email AS buyer_email "
                "FROM orders o "
                "JOIN users u ON u.id = o.buyer_id "
                "ORDER BY o.created_at DESC");
        }
        else
        {
            rows = db->execSqlSync(
                "SELECT DISTINCT o.id, o.status, "
                "o.total_amount_cents, o.created_at, "
                "o.buyer_id, u.email AS buyer_email "
                "FROM orders o "
                "JOIN users u ON u.id = o.buyer_id "
                "JOIN order_items oi ON oi.order_id = o.id "
                "JOIN products p ON p.id = oi.product_id "
                "WHERE p.seller_id = $1 "
                "ORDER BY o.created_at DESC",
                user.id);
        }

        Json::Value orders(Json::arrayValue);

        for (const auto& row : rows)
        {
            Json::Value order;
            order["id"] = row["id"].as<int>();
            order["status"] = row["status"].as<std::string>();
            order["total_amount_cents"] =
                static_cast<Json::Int64>(row["total_amount_cents"].as<int>());
            order["created_at"] = row["created_at"].as<std::string>();
            order["buyer_id"] = row["buyer_id"].as<int>();
            order["buyer_email"] = row["buyer_email"].as<std::string>();
            orders.append(order);
        }

        Json::Value data;
        data["orders"] = orders;

        Json::Value body;
        body["success"] = true;
        body["data"] = data;
        callback(jsonResponse(body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;
        body["success"] = false;
        body["message"] = std::string("Failed to load seller orders: ") + e.what();
        callback(jsonResponse(body, drogon::k500InternalServerError));
    }
}

void OrderController::getSellerOrdersOptions(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();
    const auto origin = request->getHeader("Origin");

    if (origin == "http://127.0.0.1:5500" ||
        origin == "http://localhost:5500")
    {
        response->addHeader("Access-Control-Allow-Origin", origin);
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

void OrderController::getOrder(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int userId,
    int orderId)
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

    if (userId <= 0 ||
        orderId <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Invalid user ID or order ID";

        callback(
            jsonResponse(
                body,
                drogon::k400BadRequest));

        return;
    }

    auto db =
        drogon::app().getDbClient("kanimart");

    try
    {
        auto orderRows =
            db->execSqlSync(
                "SELECT id, status, total_amount_cents, created_at "
                "FROM orders "
                "WHERE id = $1 AND buyer_id = $2",
                orderId,
                userId);

        if (orderRows.empty())
        {
            Json::Value body;

            body["success"] =
                false;

            body["message"] =
                "Order not found";

            callback(
                jsonResponse(
                    body,
                    drogon::k404NotFound));

            return;
        }

        auto itemRows =
            db->execSqlSync(
                "SELECT oi.product_id, p.name, oi.quantity, "
                "oi.unit_price_cents, "
                "(oi.quantity * oi.unit_price_cents) AS subtotal_cents "
                "FROM order_items oi "
                "JOIN products p ON p.id = oi.product_id "
                "WHERE oi.order_id = $1 "
                "ORDER BY oi.id",
                orderId);

        Json::Value items(
            Json::arrayValue);

        for (const auto& row : itemRows)
        {
            Json::Value item;

            item["product_id"] =
                row["product_id"].as<int>();

            item["name"] =
                row["name"].as<std::string>();

            item["quantity"] =
                row["quantity"].as<int>();

            item["unit_price_cents"] =
                static_cast<Json::Int64>(
                    row["unit_price_cents"].as<int>());

            item["subtotal_cents"] =
                static_cast<Json::Int64>(
                    row["subtotal_cents"].as<int>());

            items.append(item);
        }

        const auto& order =
            orderRows[0];

        Json::Value data;

        data["id"] =
            order["id"].as<int>();

        data["status"] =
            order["status"].as<std::string>();

        data["total_amount_cents"] =
            static_cast<Json::Int64>(
                order["total_amount_cents"].as<int>());

        data["created_at"] =
            order["created_at"].as<std::string>();

        data["items"] =
            items;

        Json::Value body;

        body["success"] =
            true;

        body["data"] =
            data;

        callback(
            jsonResponse(body));
    }
    catch (const std::exception& e)
    {
        Json::Value body;

        body["success"] =
            false;

        body["message"] =
            std::string(
                "Failed to load order: ") +
            e.what();

        callback(
            jsonResponse(
                body,
                drogon::k500InternalServerError));
    }
}
void OrderController::updateStatus(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    int userId,
    int orderId)
{
    AuthMiddleware::User user;

    if (!authenticateUser(request, callback, user))
    {
        return;
    }

    if (user.role != "ADMIN" && user.role != "SELLER")
    {
        callback(AuthMiddleware::forbidden());
        return;
    }

    if (userId <= 0 || orderId <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Invalid user ID or order ID";

        callback(
            jsonResponse(
                body,
                drogon::k400BadRequest));

        return;
    }

    const auto json = request->getJsonObject();

    if (!json ||
        !json->isMember("status") ||
        !(*json)["status"].isString())
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Status is required";

        callback(
            jsonResponse(
                body,
                drogon::k400BadRequest));

        return;
    }

    const std::string newStatus =
        (*json)["status"].asString();

    if (newStatus != "PENDING" &&
        newStatus != "CONFIRMED" &&
        newStatus != "SHIPPED" &&
        newStatus != "DELIVERED" &&
        newStatus != "CANCELLED")
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Invalid order status";

        callback(
            jsonResponse(
                body,
                drogon::k400BadRequest));

        return;
    }

    auto db =
        drogon::app().getDbClient("kanimart");

    try
    {
        drogon::orm::Result rows;

        if (user.role == "ADMIN")
        {
            rows = db->execSqlSync(
                "SELECT id, status FROM orders WHERE id = $1",
                orderId);
        }
        else
        {
            rows = db->execSqlSync(
                "SELECT o.id, o.status "
                "FROM orders o "
                "JOIN order_items oi ON oi.order_id = o.id "
                "JOIN products p ON p.id = oi.product_id "
                "WHERE o.id = $1 AND p.seller_id = $2",
                orderId,
                user.id);
        }

        if (rows.empty())
        {
            Json::Value body;

            body["success"] = false;
            body["message"] =
                "Order not found";

            callback(
                jsonResponse(
                    body,
                    drogon::k404NotFound));

            return;
        }

        const std::string currentStatus =
            rows[0]["status"].as<std::string>();

        bool validTransition = false;

        if (currentStatus == "PENDING" &&
            (newStatus == "CONFIRMED" ||
             newStatus == "CANCELLED"))
        {
            validTransition = true;
        }
        else if (currentStatus == "CONFIRMED" &&
                 (newStatus == "SHIPPED" ||
                  newStatus == "CANCELLED"))
        {
            validTransition = true;
        }
        else if (currentStatus == "SHIPPED" &&
                 newStatus == "DELIVERED")
        {
            validTransition = true;
        }
        else if (currentStatus == newStatus)
        {
            validTransition = true;
        }

        if (!validTransition)
        {
            Json::Value body;

            body["success"] = false;
            body["message"] =
                "Invalid status transition";

            callback(
                jsonResponse(
                    body,
                    drogon::k400BadRequest));

            return;
        }

        auto updatedRows =
            db->execSqlSync(
                "UPDATE orders "
                "SET status = $1 "
                "WHERE id = $2 "
                "RETURNING id, buyer_id, status, "
                "total_amount_cents, created_at",
                newStatus,
                orderId);

        const auto& row = updatedRows[0];

        Json::Value data;

        data["id"] =
            row["id"].as<int>();

        data["buyer_id"] =
            row["buyer_id"].as<int>();

        data["status"] =
            row["status"].as<std::string>();

        data["total_amount_cents"] =
            static_cast<Json::Int64>(
                row["total_amount_cents"].as<int>());

        data["created_at"] =
            row["created_at"].as<std::string>();

        Json::Value body;

        body["success"] = true;
        body["data"] = data;

        callback(
            jsonResponse(body));
    }
    catch (const std::exception&)
    {
        Json::Value body;

        body["success"] = false;
        body["message"] =
            "Failed to update order status";

        callback(
            jsonResponse(
                body,
                drogon::k500InternalServerError));
    }
}
void OrderController::checkoutOptions(
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
void OrderController::getOrdersOptions(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();

    const auto origin = request->getHeader("Origin");

    if (origin == "http://127.0.0.1:5500" ||
        origin == "http://localhost:5500")
    {
        response->addHeader("Access-Control-Allow-Origin", origin);
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

void OrderController::getOrderOptions(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();

    const auto origin = request->getHeader("Origin");

    if (origin == "http://127.0.0.1:5500" ||
        origin == "http://localhost:5500")
    {
        response->addHeader("Access-Control-Allow-Origin", origin);
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

void OrderController::updateStatusOptions(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto response = drogon::HttpResponse::newHttpResponse();

    const auto origin = request->getHeader("Origin");

    if (origin == "http://127.0.0.1:5500" ||
        origin == "http://localhost:5500")
    {
        response->addHeader("Access-Control-Allow-Origin", origin);
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