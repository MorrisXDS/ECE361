//
// Created by MorrisSun on 2024-01-22.
//
#include "session.h"

void create_session_status(session_status_t* session, char * session_id){
    strcpy(session->session_id, session_id);
    session->user_count = 1;
};

session_list_t* session_list_init(){
    session_list_t* session_list = (session_list_t*)malloc(sizeof(session_list_t));
    if (session_list) {
        session_list->head = NULL;
    }
    return session_list;
};

void session_list_insert_at_head(session_list_t * list,
                                            session_status_t * session_status){
    session_t * new_session = (session_t*)malloc(sizeof(session_t));
    if (new_session) {
        new_session->status = session_status;
        new_session->next = list->head;
        list->head = new_session;
    }
};

void session_list_remove(session_list_t * list, session_status_t * session_status){
    session_t * current = list->head;
    session_t * previous = NULL;
    while (current) {
        if (current->status == session_status) {
            if (previous) {
                previous->next = current->next;
            } else {
                list->head = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
};

void session_list_remove_all(session_list_t * list){
    session_t * current = list->head;
    session_t * next = NULL;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
};

void session_list_print(session_list_t * list){
    session_t * current = list->head;
    while (current) {
        printf("%s\n", current->status->session_id);
        current = current->next;
    }
};

session_status_t * session_list_find(session_list_t * list, char * session_id){
    session_t * current = list->head;
    while (current) {
        if (strcmp(current->status->session_id, session_id) == 0) {
            return current->status;
        }
        current = current->next;
    }
    return NULL;
};

int session_list_count(session_list_t * list){
    session_t * current = list->head;
    int count = 0;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
};