cmake_minimum_required(VERSION 3.25)
project(MySQLTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(mysql_test mysql_test.cpp)
target_include_directories(mysql_test PRIVATE
    "C:/kanimart/vcpkg_installed/x64-windows/include/mysql"
)
target_link_directories(mysql_test PRIVATE
    "C:/kanimart/vcpkg_installed/x64-windows/lib"
)
target_link_libraries(mysql_test PRIVATE
    libmariadb.lib
)
