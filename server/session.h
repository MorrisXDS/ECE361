#ifndef TCL_SESSION_H
#define TCL_SESSION_H
#include "../message/message.h"

#define session_t struct session
#define session_list_t struct session_list
#define session_status_t struct session_status

#define MAX_SESSION_NAME 20
#define MAX_SESSION_CAPACITY 20

session_status_t{
    char session_id[MAX_SESSION_NAME];
    unsigned int user_count;
};

session_t{
    session_status_t * status;
    session_t * next;
};

session_list_t{
    session_t * head;
};

void create_session_status(session_status_t* session, char * session_id);

session_list_t* session_list_init();
void session_list_insert_at_head(session_list_t * list,
                                            session_status_t * session_status);
void session_list_remove(session_list_t * list, session_status_t * session_status);
void session_list_remove_all(session_list_t * list);
void session_list_print(session_list_t * list);
session_status_t * session_list_find(session_list_t * list, char * session_id);
int session_list_count(session_list_t * list);


#endif //TCL_SESSION_H
