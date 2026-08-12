#include "UserService.h"

bool UserService::registerUser(const User &user)
{
    users.push_back(user);
    return true;
}

std::vector<User> UserService::getAllUsers()
{
    return users;
}