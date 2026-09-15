#pragma once
#include <string>
namespace kani::kanimart {
class JwtService final
{
public:
    static std::string generateToken(
        int userId,
        const std::string& role);
    static bool verifyToken(
        const std::string& token,
        int& userId,
        std::string& role);
};
} // namespace kani::kanimart
