//
// Created by MorrisSun on 2024-01-23.
//
#ifndef USER_H
#define USER_H
#include "../message/message.h"
#include <pthread.h>

#define MAX_SESSION_COUNT 10

#define USER_CREATE_SUCCESS 1
#define USER_ALREADY_EXIST 2
#define ADD_USER_SUCCESS 3
#define USER_LIST_FULL 4
#define MAX_SESSION_USER 6
#define MAX_USER_COUNT 100

#define SESSION_NAME_INVALID (-2)
#define SESSION_AT_FULL_CAPACITY  (-3)
#define SESSION_EXISTS (-4)
#define SESSION_ADD_USER_FAILURE (-5)
#define USER_NAME_IN_SESSION (-6)
#define USER_DOES_NOT_EXIST (-7)
#define USER_NOT_IN_SESSION (-8)
#define USER_NOT_ONLINE (-9)
#define USER_IN_DIFFERENT_SESSION (-10)
#define TARGET_IS_ADMIN (-11)

typedef struct user{
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_PASSWORD_LENGTH];
    char session_id[MAX_SESSION_LENGTH];
    char ip_address[INET6_ADDRSTRLEN]; //compatibility for both IPv4 and IPv6
    unsigned char in_session;
    unsigned char status;
    unsigned int port;
    int socket_fd;
    char role;
} user_t;

typedef struct user_list{
    user_t * user_list;
    unsigned int count;
    unsigned int capacity;
} user_database;


user_database* user_database_init();
int create_user(user_t * user, user_t * user_to_add);
int user_database_add_user(user_database * list, user_t * user_data, pthread_mutex_t * mutex);
void remove_user_by_name_in_database(user_database * list, char * name, pthread_mutex_t * mutex);
void remove_all_users_in_database(user_database * list, pthread_mutex_t * mutex);
void print_all_users_in_a_database(user_database * list);
void print_active_users_in_a_database(user_database * list);
int get_active_user_list_in_a_session(user_database* list,
                                     char * active_user_list, char * session_id,
                                     pthread_mutex_t *mutex);
void get_active_user_list_in_database(user_database* list, char * active_user_list,  pthread_mutex_t *mutex);
user_t * database_search_user(user_database* list, unsigned char * username);
int get_user_count_in_database(user_database* list, pthread_mutex_t * mutex);
int get_user_count_in_a_session(user_database* list, char * session_id, pthread_mutex_t * mutex);
user_t* return_user_by_username(user_database* list, unsigned char * username);
void user_exit_current_session(user_database* list, char * username, pthread_mutex_t * mutex);

#endif //USER_H
