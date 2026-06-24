#ifndef PRESENTATION_H
# define PRESENTATION_H

# include <stdio.h>
# include <stdlib.h>

typedef struct Auth Auth;

void print_menu(Auth *auth);
int menu_login(Auth *auth);
int menu_unblock_user(Auth *auth);
int menu_block_user(Auth *auth);
int menu_find_user(Auth *auth);
int menu_remove_user(Auth *auth);
int menu_create_user(Auth *auth);

#endif