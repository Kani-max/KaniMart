#include "AuthController.h"

#include <drogon/drogon.h>
#include <sodium.h>

#include "../services/JwtService.h"

#include <array>
#include <string>

namespace kani::kanimart
{

namespace
{

// ============================================================
// Initialize libsodium
// ============================================================

bool initializeSodium()
{
    return sodium_init() >= 0;
}


// ============================================================
// Hash password using Argon2id
// ============================================================

bool hashPassword(
    const std::string& password,
    std::string& passwordHash)
{
    std::array<char, crypto_pwhash_STRBYTES> hash{};

    if (crypto_pwhash_str_alg(
            hash.data(),
            password.c_str(),
            password.size(),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE,
            crypto_pwhash_ALG_ARGON2ID13) != 0)
    {
        return false;
    }

    passwordHash = hash.data();

    return true;
}


// ============================================================
// Verify password against Argon2id hash
// ============================================================

bool verifyPassword(
    const std::string& passwordHash,
    const std::string& password)
{
    return crypto_pwhash_str_verify(
               passwordHash.c_str(),
               password.c_str(),
               password.size()) == 0;
}

} // namespace


// ============================================================
// REGISTER USER
// ============================================================

void AuthController::registerUser(
    const drogon::HttpRequestPtr& request,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        Json::Value response;


        // ----------------------------------------------------
        // Initialize password security
        // ----------------------------------------------------

        if (!initializeSodium())
        {
            response["success"] = false;
            response["error"] =
                "Password security initialization failed";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Read JSON
        // ----------------------------------------------------

        auto json = request->getJsonObject();

        if (!json)
        {
            response["success"] = false;
            response["error"] =
                "Invalid JSON request";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k400BadRequest);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Read registration fields
        // ----------------------------------------------------

        const std::string name =
            (*json)["name"].asString();

        const std::string email =
            (*json)["email"].asString();

        const std::string password =
            (*json)["password"].asString();

        const std::string role =
            (*json)["role"].asString();


        // ----------------------------------------------------
        // Validate required fields
        // ----------------------------------------------------

        if (name.empty() ||
            email.empty() ||
            password.empty() ||
            role.empty())
        {
            response["success"] = false;
            response["error"] =
                "Name, email, password and role are required";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k400BadRequest);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Validate password length
        // ----------------------------------------------------

        if (password.size() < 8)
        {
            response["success"] = false;
            response["error"] =
                "Password must be at least 8 characters";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k400BadRequest);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Validate registration role
        //
        // IMPORTANT:
        // ADMIN is intentionally NOT allowed through
        // public registration.
        // ----------------------------------------------------

        if (role != "BUYER" &&
            role != "SELLER")
        {
            response["success"] = false;
            response["error"] =
                "Registration role must be BUYER or SELLER";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k400BadRequest);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Hash password
        // ----------------------------------------------------

        std::string passwordHash;

        if (!hashPassword(
                password,
                passwordHash))
        {
            response["success"] = false;
            response["error"] =
                "Password hashing failed";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Database connection
        // ----------------------------------------------------

        auto db =
            drogon::app().getDbClient(
                "kanimart");


        // ----------------------------------------------------
        // Insert user
        //
        // Parameterized SQL prevents SQL injection.
        // ----------------------------------------------------

        auto result = db->execSqlSync(
            "INSERT INTO users "
            "(name, email, password_hash, role) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id, name, email, role",

            name,
            email,
            passwordHash,
            role
        );


        // ----------------------------------------------------
        // Read inserted user
        // ----------------------------------------------------

        const auto& row =
            result[0];


        // ----------------------------------------------------
        // Build successful response
        // ----------------------------------------------------

        response["success"] = true;

        response["message"] =
            "Registration successful";


        response["data"]["user"]["id"] =
            row["id"].as<int>();

        response["data"]["user"]["name"] =
            row["name"].as<std::string>();

        response["data"]["user"]["email"] =
            row["email"].as<std::string>();

        response["data"]["user"]["role"] =
            row["role"].as<std::string>();


        // ----------------------------------------------------
        // Return HTTP 201
        // ----------------------------------------------------

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        resp->setStatusCode(
            drogon::k201Created);

        callback(resp);
    }


    // ========================================================
    // Database / unexpected error
    // ========================================================

    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;

        const std::string error =
            exception.what();


        // ----------------------------------------------------
        // Duplicate email
        // ----------------------------------------------------

        if (error.find("users_email_key")
            != std::string::npos)
        {
            response["error"] =
                "Email already registered";
        }
        else
        {
            response["error"] =
                "Database error";
        }


        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(
                response);


        if (error.find("users_email_key")
            != std::string::npos)
        {
            resp->setStatusCode(
                drogon::k409Conflict);
        }
        else
        {
            resp->setStatusCode(
                drogon::k500InternalServerError);
        }


        callback(resp);
    }
}


// ============================================================
// LOGIN
// ============================================================

void AuthController::login(
    const drogon::HttpRequestPtr& request,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        Json::Value response;


        // ----------------------------------------------------
        // Initialize libsodium
        // ----------------------------------------------------

        if (!initializeSodium())
        {
            response["success"] = false;
            response["error"] =
                "Password security initialization failed";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Read JSON
        // ----------------------------------------------------

        auto json =
            request->getJsonObject();

        if (!json)
        {
            response["success"] = false;
            response["error"] =
                "Invalid JSON request";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k400BadRequest);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Read login fields
        // ----------------------------------------------------

        const std::string email =
            (*json)["email"].asString();

        const std::string password =
            (*json)["password"].asString();


        // ----------------------------------------------------
        // Validate login fields
        // ----------------------------------------------------

        if (email.empty() ||
            password.empty())
        {
            response["success"] = false;
            response["error"] =
                "Email and password are required";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k400BadRequest);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Database connection
        // ----------------------------------------------------

        auto db =
            drogon::app().getDbClient(
                "kanimart");


        // ----------------------------------------------------
        // Find user
        // ----------------------------------------------------

        auto result = db->execSqlSync(
            "SELECT id, name, email, password_hash, role "
            "FROM users "
            "WHERE email = $1",
            email
        );


        // ----------------------------------------------------
        // User not found
        // ----------------------------------------------------

        if (result.empty())
        {
            response["success"] = false;
            response["error"] =
                "Invalid email or password";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k401Unauthorized);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Read database row
        // ----------------------------------------------------

        const auto& row =
            result[0];


        const std::string storedPasswordHash =
            row["password_hash"]
                .as<std::string>();


        // ----------------------------------------------------
        // Verify Argon2id password
        // ----------------------------------------------------

        if (!verifyPassword(
                storedPasswordHash,
                password))
        {
            response["success"] = false;
            response["error"] =
                "Invalid email or password";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k401Unauthorized);

            callback(resp);

            return;
        }


        // ----------------------------------------------------
        // Read user identity
        // ----------------------------------------------------

        const int userId =
            row["id"].as<int>();


        const std::string role =
            row["role"].as<std::string>();


        // ----------------------------------------------------
        // Generate JWT
        // ----------------------------------------------------

        const std::string token =
            JwtService::generateToken(
                userId,
                role);


        // ----------------------------------------------------
        // Successful response
        // ----------------------------------------------------

        response["success"] = true;

        response["message"] =
            "Login successful";


        response["data"]["token"] =
            token;


        response["data"]["user"]["id"] =
            userId;


        response["data"]["user"]["name"] =
            row["name"].as<std::string>();


        response["data"]["user"]["email"] =
            row["email"].as<std::string>();


        response["data"]["user"]["role"] =
            role;


        // ----------------------------------------------------
        // Return HTTP 200
        // ----------------------------------------------------

        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        resp->setStatusCode(
            drogon::k200OK);

        callback(resp);
    }


    // ========================================================
    // Unexpected error
    // ========================================================

    catch (const std::exception& exception)
    {
        Json::Value response;

        response["success"] = false;

        response["error"] =
            exception.what();


        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(
                response);

        resp->setStatusCode(
            drogon::k500InternalServerError);

        callback(resp);
    }
}


// ============================================================
// CORS OPTIONS
// ============================================================

void AuthController::options(
    const drogon::HttpRequestPtr& request,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    auto response =
        drogon::HttpResponse::newHttpResponse();


    const auto origin =
        request->getHeader("Origin");


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


    response->setStatusCode(
        drogon::k200OK);


    callback(response);
}

} // namespace kani::kanimart