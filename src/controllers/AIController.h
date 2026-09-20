#pragma once
#include <drogon/HttpController.h>
namespace kani::kanimart
{
class AIController final
    : public drogon::HttpController<AIController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        AIController::chat,
        "/api/chat",
        drogon::Post);
    ADD_METHOD_TO(
        AIController::options,
        "/api/chat",
        drogon::Options);
    METHOD_LIST_END
    void chat(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void options(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
}  // namespace kani::kanimart
