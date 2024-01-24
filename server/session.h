#ifndef TCL_SESSION_H
#define TCL_SESSION_H
#include "../message/message.h"
#include "user.h"

#define session_t struct session
#define session_list_t struct session_list
#define session_status_t struct session_status

#define MAX_SESSION_NAME 20
#define MAX_SESSION_CAPACITY 20
#define MAXIMUM_USER_COUNT 20

#define SESSION_CREATE_FAIL 1
#define SESSION_NAME_RESERVED 2
#define SESSION_NAME_INVALID 3
#define SESSION_NAME_TOO_LONG 4
#define SESSION_LIST_FULL  5
#define SESSION_EXISTS 6
#define SESSION_ADD_USER_SUCCESS 7
#define SESSION_ADD_USER_FAILURE 8
#define SESSION_CREATE_SUCCESS 9

session_status_t{
    char session_id[MAX_SESSION_NAME];
    user_list_t* user_list;
};

session_t{
    session_status_t * session;
    session_t * next;
};

session_list_t{
    session_t * head;
};

int create_session_status(session_status_t* session, char * session_id, user_t* user);
session_list_t* session_list_init();
int session_list_insert_at_tail(session_list_t * list,
                                            session_status_t * session);
void session_list_remove(session_list_t * list, session_status_t * session);
void session_list_remove_all(session_list_t * list);
void session_list_print(session_list_t * list);
session_status_t * session_list_find(session_list_t * list, char * session_id);
int session_list_count(session_list_t * list);
int session_list_session_add_user(session_list_t * list, char * session_id, user_t* user);
void session_list_remove_user(session_list_t * list, char * session_id, char* name);
int session_user_count(session_list_t * list, char * session_id);
void print_session_user_list(session_list_t * list, char * session_id);

#endif //TCL_SESSION_H
