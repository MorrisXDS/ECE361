//
// Created by MorrisSun on 2024-01-22.
//
#include "session.h"

int create_session_status(session_status_t* session, char * session_id, user_t* user){
    if (session == NULL) {
        fprintf(stderr, "Session is NULL\n");
        return SESSION_NAME_INVALID;
    }
    if (strcmp(session_id, NOT_IN_SESSION) == 0){
        fprintf(stderr, "id \"not in session\" is reserved. please select a different ID\n");
        return SESSION_NAME_RESERVED;
    }
    if (strlen(session_id) >= MAX_SESSION_NAME){
        fprintf(stderr, "Session name too long to fit\n");
        return SESSION_NAME_TOO_LONG;
    }
    strcpy(session->session_id, session_id);
    session->user_list = malloc(sizeof(user_t) * MAXIMUM_USER_COUNT);
    user_list_add_user(session->user_list, *user);
    return SESSION_CREATE_SUCCESS;
}

session_list_t* session_list_init(){
    session_list_t* session_list = (session_list_t*)malloc(sizeof(session_list_t));
    if (session_list) {
        session_list->head = NULL;
    }
    return session_list;
}

int session_list_insert_at_tail(session_list_t * list,
                                            session_status_t * session){
    if (session == NULL) {
        fprintf(stderr, "Session is NULL\n");
        return SESSION_NAME_INVALID;
    }
    session_t * new_session = malloc(sizeof(session_t));
    if (new_session == NULL) {
        fprintf(stderr, "Failed to allocate memory for new session\n");
        return SESSION_CREATE_FAIL;
    }
    new_session->session = malloc(sizeof(session_status_t));
    if (new_session->session == NULL) {
        fprintf(stderr, "Failed to allocate memory for new session\n");
        return SESSION_CREATE_FAIL;
    }
    strcpy(new_session->session->session_id, session->session_id);
    new_session->session->user_list = malloc(sizeof(user_list_t));
    if (new_session->session->user_list == NULL) {
        fprintf(stderr, "Failed to allocate memory for new session\n");
        return SESSION_CREATE_FAIL;
    }
    new_session->session->user_list->head = NULL;
    new_session->next = NULL;

    if (list->head == NULL) {
        list->head = new_session;
    } else {
        session_t * current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_session;
    }
    return 0;
}

void session_list_remove(session_list_t * list, session_status_t * session){
    if (session == NULL) {
        fprintf(stderr, "Session session is NULL\n");
        return;
    }
    session_t * current = list->head;
    session_t * previous = NULL;
    while (current != NULL) {
        if (strcmp(current->session->session_id, session->session_id) == 0) {
            if (previous) {
                previous->next = current->next;
            } else {
                list->head = current->next;
            }
            user_list_remove_all(current->session->user_list);
            free(current->session);
            free(current);
            break;
        }
        previous = current;
        current = current->next;
    }
    session_list_print(list);
}

void session_list_remove_all(session_list_t * list){
    session_t * current = list->head;
    session_t * next = NULL;
    while (current != NULL) {
        next = current->next;
        user_list_remove_all(current->session->user_list);
        free(current->session);
        free(current);
        current = next;
    }
    free(list);
}

void session_list_print(session_list_t * list){
    session_t * current = list->head;
    while (current != NULL) {
        printf("session name: %s ", current->session->session_id);
        fprintf(stdout,"user count: %d\n", user_list_count(current->session->user_list));
        current = current->next;
    }
}

session_status_t * session_list_find(session_list_t * list, char * session_id){
    if (session_id == NULL) {
        fprintf(stderr, "Session id is NULL\n");
        return NULL;
    }
    session_t * current = list->head;
    fprintf(stdout, "session count: %d\n", session_list_count(list));
    while (current != NULL) {
        fprintf(stdout, "registered session id: %s\n", current->session->session_id);
        fprintf(stdout, "passed in session id: %s\n", session_id);
        if (strcmp(current->session->session_id, session_id) == 0) {
            return current->session;
        }
        current = current->next;
    }
    session_list_print(list);
    return NULL;
}

int session_list_count(session_list_t * list){
    session_t * current = list->head;
    int count = 0;
    while (current) {
        count++;
        current = current->next;
    }
    session_list_print(list);
    return count;
}

int session_list_session_add_user(session_list_t * list, char * session_id, user_t* user){
    if (session_id == NULL) {
        fprintf(stderr, "Session id is NULL\n");
        return SESSION_NAME_INVALID;
    }
    if (strcmp(session_id, NOT_IN_SESSION) == 0){
        fprintf(stderr, "\"not in session\" is not a reserved name, not a session\n");
        return SESSION_NAME_RESERVED;
    }
    session_t * current = list->head;
    while (current != NULL) {
        if (strcmp(current->session->session_id, session_id) == 0) {
            int result = user_list_add_user(current->session->user_list, *user);
            if(result != ADD_USER_SUCCESS){
                return SESSION_ADD_USER_FAILURE;
            }
            return SESSION_ADD_USER_SUCCESS;
        }
        current = current->next;
    }
    return SESSION_NAME_INVALID;
}

int session_user_count(session_list_t * list, char * session_id){
    if (session_id == NULL) {
        fprintf(stderr, "Session id is NULL\n");
        return -1;
    }
    session_t * current = list->head;
    while (current) {
        if (strcmp(current->session->session_id, session_id) == 0) {
            return user_list_count(current->session->user_list);
        }
        current = current->next;
    }
    return -1;
}

void print_session_user_list(session_list_t * list, char * session_id){
    if (session_id == NULL) {
        fprintf(stderr, "Session id is NULL\n");
        return;
    }
    session_t * current = list->head;
    fprintf(stdout, "Session %s user list:\n", session_id);
    fprintf(stdout, "%-17s %s", "username", "role");
    while (current != NULL) {
        if (strcmp(current->session->session_id, session_id) == 0) {
            user_list_print(current->session->user_list);
            return;
        }
        current = current->next;
    }
}

void session_list_remove_user(session_list_t * list, char * session_id, char* name){
    if (session_id == NULL) {
        fprintf(stderr, "Session id is NULL\n");
        return;
    }
    session_t * current = list->head;
    while (current) {
        if (strcmp(current->session->session_id, session_id) == 0) {
            user_list_remove_by_name(current->session->user_list, name);
            return;
        }
        current = current->next;
    }
}