#include <iostream>
#include <mysql/mysql.h>

int main()
{
    MYSQL* conn = mysql_init(nullptr);

    if (!conn)
    {
        std::cout << "mysql_init FAILED\n";
        return 1;
    }

    std::cout << "mysql_init OK\n";

    if (!mysql_real_connect(
        conn,
        "127.0.0.1",
        "kanimart_app",
        "KaniMart@2026",
        "kanimart",
        3306,
        nullptr,
        0))
    {
        std::cout << "mysql_real_connect FAILED\n";
        std::cout << "Error: " << mysql_error(conn) << "\n";
        mysql_close(conn);
        return 2;
    }

    std::cout << "MYSQL CONNECTION SUCCESS!\n";

    if (mysql_query(conn, "SELECT 1"))
    {
        std::cout << "Query FAILED: " << mysql_error(conn) << "\n";
    }
    else
    {
        std::cout << "Query SUCCESS!\n";
    }

    mysql_close(conn);
    return 0;
}