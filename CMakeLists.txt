cmake_minimum_required(VERSION 3.25)

project(
    KaniMart
    VERSION 0.1.0
    DESCRIPTION "Native C++20 Multi-Seller E-Commerce Marketplace"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(kanimart
    src/main.cpp

    src/controllers/AuthController.cpp
    src/controllers/HomeController.cpp
    src/controllers/ProductController.cpp
    src/controllers/UserController.cpp

    src/services/UserService.cpp
)

target_link_libraries(kanimart
    PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

target_include_directories(kanimart
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(kanimart PRIVATE /W4)
else()
    target_compile_options(kanimart PRIVATE -Wall -Wextra -Wpedantic)
endif()