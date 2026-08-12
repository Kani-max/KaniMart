#pragma once

#include "../models/User.h"
#include <vector>

class UserService
{
public:
    bool registerUser(const User &user);

    std::vector<User> getAllUsers();

private:
    std::vector<User> users;
};