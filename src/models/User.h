#pragma once

#include <string>

class User
{
public:
    int id;
    std::string name;
    std::string email;
    std::string password;
    std::string role;

    User()
    {
        id = 0;
        role = "customer";
    }
};