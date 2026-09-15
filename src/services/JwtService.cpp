#include "JwtService.h"
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
namespace kani::kanimart
{
namespace
{
std::string getJwtSecret()
{
    char* secret = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(
            &secret,
            &size,
            "KANIMART_JWT_SECRET") != 0 ||
        secret == nullptr ||
        size == 0)
    {
        if (secret != nullptr)
        {
            free(secret);
        }
        throw std::runtime_error(
            "KANIMART_JWT_SECRET environment variable is not set");
    }
    std::string result(secret);
    free(secret);
    return result;
}
} // namespace
std::string JwtService::generateToken(
    int userId,
    const std::string& role)
{
    using traits = jwt::traits::kazuho_picojson;
    const auto now =
        std::chrono::system_clock::now();
    const auto expiry =
        now + std::chrono::hours(24);
    return jwt::create<traits>()
        .set_issuer("kanimart")
        .set_subject(std::to_string(userId))
        .set_payload_claim(
            "role",
            traits::value_type(role))
        .set_issued_at(now)
        .set_expires_at(expiry)
        .sign(
            jwt::algorithm::hs256{
                getJwtSecret()});
}
bool JwtService::verifyToken(
    const std::string& token,
    int& userId,
    std::string& role)
{
    try
    {
        using traits = jwt::traits::kazuho_picojson;
        const auto decoded =
            jwt::decode<traits>(token);
        auto verifier =
            jwt::verify<jwt::default_clock, traits>(
                jwt::default_clock{})
                .allow_algorithm(
                    jwt::algorithm::hs256{
                        getJwtSecret()})
                .with_issuer("kanimart");
        verifier.verify(decoded);
        const auto subject =
            decoded.get_subject();
        userId = std::stoi(subject);
        role =
            decoded.get_payload_claim("role")
                .as_string();
        return true;
    }
    catch (...)
    {
        return false;
    }
}
} // namespace kani::kanimart
