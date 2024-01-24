//
// Created by MorrisSun on 2024-01-23.
//



#ifndef USER_H
#define USER_H
#include "../message/message.h"


#define user_t struct user
#define user_list_t struct user_list
#define link_user_t struct link_user
#define MAX_PASSWORD_LENGTH 20
#define MAX_SESSION_COUNT 20
#define NOT_IN_SESSION "Not in session"
#define IP_NOT_FOUND "ip not found"
#define ADMIN 1
#define USER 0
#define USER_CREATE_SUCCESS 1
#define USER_CREATE_FAIL 0
#define ADD_USER_SUCCESS 3
#define ADD_USER_FAILURE 4
#define USER_LIST_FULL 5
#define MAX_USER_COUNT 100



user_t{
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_PASSWORD_LENGTH];
    unsigned char status;
    char session_id[MAX_SESSION_COUNT];
    char ip_address[INET6_ADDRSTRLEN]; //compatibility for both IPv4 and IPv6
    unsigned int port;
    int socket_fd;
    char role;
};

link_user_t{
    user_t * user;
    link_user_t * next;
};

user_list_t{
    link_user_t * head;
};

user_list_t* user_list_init();
int create_user(user_t * user, unsigned char * username, unsigned char * password,
                 unsigned char status, char * session_id, char * ip_address, unsigned int port, int socket_fd, char role);
int user_list_add_user(user_list_t * list, user_t user);
void user_list_remove_by_name(user_list_t * list, char * name);
void user_list_remove_all(user_list_t * list);
void user_list_print(user_list_t * list);
user_t * user_list_find(user_list_t * list, unsigned char * username);
int user_list_count(user_list_t * list);
int user_list_count_by_session_id(user_list_t * list, char * session_id);
user_t* return_user_by_username(user_list_t * list, unsigned char * username);


#endif //USER_H
