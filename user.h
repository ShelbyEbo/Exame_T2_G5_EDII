#ifndef USER_H
# define USER_H

# include <stdlib.h>
# include <string.h>
# include "file.h"

typedef struct User
{
    int     id;
    int     blocked;
    char    name[100];
    File    *files;
    // Message *inbox;
    // Share   *shares;
} User;

User    *create_user(int id, char *name);
User    *destroy_user(User *user);

#endif