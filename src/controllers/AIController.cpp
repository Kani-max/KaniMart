#include "AIController.h"

#include <drogon/HttpClient.h>
#include <drogon/drogon.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>

namespace kani::kanimart
{

void AIController::options(
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
            "POST, OPTIONS");

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


void AIController::chat(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        const char* apiKey =
            std::getenv("OPENAI_API_KEY");

        if (!apiKey || std::string(apiKey).empty())
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "AI service is not configured.";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k503ServiceUnavailable);

            callback(httpResponse);
            return;
        }


        auto jsonBody =
            request->getJsonObject();

        if (!jsonBody ||
            !jsonBody->isMember("message") ||
            !(*jsonBody)["message"].isString())
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "Message is required.";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k400BadRequest);

            callback(httpResponse);
            return;
        }


        const std::string userMessage =
            (*jsonBody)["message"].asString();

        if (userMessage.empty() ||
            userMessage.size() > 2000)
        {
            Json::Value response;

            response["success"] = false;
            response["error"] =
                "Message must contain between 1 and 2000 characters.";

            auto httpResponse =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            httpResponse->setStatusCode(
                drogon::k400BadRequest);

            callback(httpResponse);
            return;
        }


        nlohmann::json requestBody;

        requestBody["model"] =
            "gpt-5.6-luna";

        requestBody["instructions"] =
            "You are KaniMart Assistant, a helpful shopping "
            "assistant for the KaniMart multi-seller marketplace. "
            "Answer questions about shopping, products, carts, "
            "orders, reviews, sellers, and how to use KaniMart. "
            "Be concise and helpful. "
            "Do not claim to know live product stock, prices, "
            "orders, or account information unless that information "
            "is explicitly provided in the conversation.";

        requestBody["input"] =
            userMessage;


        auto client =
            drogon::HttpClient::newHttpClient(
                "https://api.openai.com");


        auto httpRequest =
            drogon::HttpRequest::newHttpRequest();

        httpRequest->setMethod(
            drogon::Post);

        httpRequest->setPath(
            "/v1/responses");

        httpRequest->setContentTypeCode(
            drogon::CT_APPLICATION_JSON);

        httpRequest->addHeader(
            "Authorization",
            std::string("Bearer ") + apiKey);

        httpRequest->setBody(
            requestBody.dump());


        client->sendRequest(
            httpRequest,

            [callback = std::move(callback)]
            (
                drogon::ReqResult result,
                const drogon::HttpResponsePtr& response
            )
            {
                /*
                 * Network / connection failure
                 */
                if (result != drogon::ReqResult::Ok ||
                    !response)
                {
                    Json::Value errorResponse;

                    errorResponse["success"] = false;
                    errorResponse["error"] =
                        "AI service is temporarily unavailable.";

                    auto httpResponse =
                        drogon::HttpResponse::newHttpJsonResponse(
                            errorResponse);

                    httpResponse->setStatusCode(
                        drogon::k503ServiceUnavailable);

                    callback(httpResponse);
                    return;
                }


                /*
                 * OpenAI returned a non-success status.
                 *
                 * Do not expose the upstream response body
                 * or API/provider details to the browser.
                 */
                if (response->getStatusCode() < 200 ||
                    response->getStatusCode() >= 300)
                {
                    Json::Value errorResponse;

                    errorResponse["success"] = false;
                    errorResponse["error"] =
                        "AI service returned an error.";

                    auto httpResponse =
                        drogon::HttpResponse::newHttpJsonResponse(
                            errorResponse);

                    httpResponse->setStatusCode(
                        drogon::k502BadGateway);

                    callback(httpResponse);
                    return;
                }


                /*
                 * Parse successful AI response.
                 */
                try
                {
                    const auto body =
                        nlohmann::json::parse(
                            response->getBody());


                    std::string answer;


                    /*
                     * Preferred Responses API output.
                     */
                    if (body.contains("output_text") &&
                        body["output_text"].is_string())
                    {
                        answer =
                            body["output_text"].get<std::string>();
                    }


                    /*
                     * Fallback: inspect output/content.
                     */
                    if (answer.empty() &&
                        body.contains("output") &&
                        body["output"].is_array())
                    {
                        for (const auto& item :
                             body["output"])
                        {
                            if (!item.contains("content") ||
                                !item["content"].is_array())
                            {
                                continue;
                            }


                            for (const auto& content :
                                 item["content"])
                            {
                                if (content.contains("text") &&
                                    content["text"].is_string())
                                {
                                    answer +=
                                        content["text"]
                                            .get<std::string>();
                                }
                            }
                        }
                    }


                    /*
                     * Empty AI response.
                     */
                    if (answer.empty())
                    {
                        Json::Value errorResponse;

                        errorResponse["success"] = false;
                        errorResponse["error"] =
                            "AI returned an empty response.";

                        auto httpResponse =
                            drogon::HttpResponse::
                                newHttpJsonResponse(
                                    errorResponse);

                        httpResponse->setStatusCode(
                            drogon::k502BadGateway);

                        callback(httpResponse);
                        return;
                    }


                    /*
                     * Successful response to frontend.
                     */
                    Json::Value jsonResponse;

                    jsonResponse["success"] = true;
                    jsonResponse["data"]["message"] =
                        answer;


                    auto httpResponse =
                        drogon::HttpResponse::
                            newHttpJsonResponse(
                                jsonResponse);

                    httpResponse->setStatusCode(
                        drogon::k200OK);

                    callback(httpResponse);
                }
                catch (const std::exception&)
                {
                    Json::Value errorResponse;

                    errorResponse["success"] = false;
                    errorResponse["error"] =
                        "Invalid response from AI service.";

                    auto httpResponse =
                        drogon::HttpResponse::
                            newHttpJsonResponse(
                                errorResponse);

                    httpResponse->setStatusCode(
                        drogon::k502BadGateway);

                    callback(httpResponse);
                }
            });
    }
    catch (const std::exception&)
    {
        Json::Value response;

        response["success"] = false;
        response["error"] =
            "Unable to process AI request.";

        auto httpResponse =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        httpResponse->setStatusCode(
            drogon::k500InternalServerError);

        callback(httpResponse);
    }
}

}  // namespace kani::kanimart