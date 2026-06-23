#ifndef AUTH_H
# define AUTH_H

#include "avl.h"

typedef struct Auth
{
    AVL     *users;
    User    *current_user;
} Auth;

Auth *init_auth();
void logout(Auth *auth);
void block_user(User *user);
void unblock_user(User *user);
void login(Auth *auth, int user_id);

#endif