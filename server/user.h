//
// Created by MorrisSun on 2024-01-23.
//

#ifndef USER_H
#define USER_H
#include "../message/message.h"
#include <pthread.h>


#define user_t struct user
#define user_database struct user_list

#define MAX_SESSION_COUNT 10
#define NOT_IN_SESSION "Not in session"

#define USER 0
#define ADMIN 1

#define OFFLINE 0
#define ONLINE 1

#define USER_CREATE_SUCCESS 1
#define USER_ALREADY_EXIST 2
#define ADD_USER_SUCCESS 3
#define USER_LIST_FULL 4
#define MAX_SESSION_USER 6
#define MAX_USER_COUNT 100

#define SESSION_NAME_RESERVED (-1)
#define SESSION_NAME_INVALID (-2)
#define SESSION_AT_FULL_CAPACITY  (-3)
#define SESSION_EXISTS (-4)
#define SESSION_ADD_USER_FAILURE (-5)
#define USER_NAME_IN_SESSION (-6)

user_t{
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_PASSWORD_LENGTH];
    char session_id[MAX_SESSION_LENGTH];
    char ip_address[INET6_ADDRSTRLEN]; //compatibility for both IPv4 and IPv6
    unsigned char in_session;
    unsigned char status;
    unsigned int port;
    int socket_fd;
    char role;
};

user_database{
    user_t * user_list;
    unsigned int count;
    unsigned int capacity;
};


user_database* user_list_init();
int create_user(user_t * user, user_t * user_to_add);
int user_list_add_user(user_database * list, user_t * user_data, pthread_mutex_t * mutex);
void user_list_remove_by_name(user_database * list, char * name, pthread_mutex_t * mutex);
void user_list_remove_all(user_database * list, pthread_mutex_t * mutex);
void user_list_print(user_database * list);
void active_user_list_print(user_database * list);
void get_active_user_list(user_database* list, char * active_user_list,  pthread_mutex_t *mutex);
user_t * user_list_find(user_database* list, unsigned char * username);
int user_list_count(user_database* list, pthread_mutex_t * mutex);
int user_list_count_by_session_id(user_database* list, char * session_id, pthread_mutex_t * mutex);
user_t* return_user_by_username(user_database* list, unsigned char * username);
void user_exit_current_session(user_database* list, char * username, pthread_mutex_t * mutex);

#endif //USER_H
