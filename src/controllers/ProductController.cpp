#include "ProductController.h"

void ProductController::list(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto dbClient = app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT id, name, description, price, category, stock, rating "
        "FROM products ORDER BY id",

        [callback](const orm::Result& result)
        {
            Json::Value response;
            response["success"] = true;
            response["count"] = static_cast<int>(result.size());

            Json::Value products(Json::arrayValue);

            for (const auto& row : result)
            {
                Json::Value product;

                product["id"] = row["id"].as<int>();
                product["name"] = row["name"].as<std::string>();
                product["description"] =
                    row["description"].isNull()
                        ? ""
                        : row["description"].as<std::string>();

                product["price"] = row["price"].as<double>();

                product["category"] =
                    row["category"].isNull()
                        ? ""
                        : row["category"].as<std::string>();

                product["stock"] = row["stock"].as<int>();
                product["rating"] = row["rating"].as<double>();

                products.append(product);
            }

            response["products"] = products;

            auto resp = HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        },

        [callback](const orm::DrogonDbException& e)
        {
            Json::Value response;

            response["success"] = false;
            response["message"] = "Failed to fetch products";
            response["error"] = e.base().what();

            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k500InternalServerError);

            callback(resp);
        });
}