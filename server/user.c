#include "user.h"

user_list_t* user_list_init(){
    user_list_t* user_list = (user_list_t*)malloc(sizeof(user_list_t));
    if (user_list){
        user_list->head = NULL;
    }
    return user_list;
}

int user_list_add_user(user_list_t * list, user_t user) {
    if (user_list_count(list) >= MAX_USER_COUNT) {
        fprintf(stderr, "User list is full\n");
        return USER_LIST_FULL;
    }
    link_user_t *new_user = (link_user_t *) malloc(sizeof(link_user_t));
    if (new_user) {
        new_user->user = malloc(sizeof(user_t));
        create_user(new_user->user, user.username, user.password,
                    user.status, user.session_id, user.ip_address,
                    user.port, user.socket_fd, user.role);
        new_user->next = list->head;
        list->head = new_user;
        fprintf(stdout, "User %s added to user list\n", user.username);
        fprintf(stdout, "Now there is %d people in session \"%s\"\n", user_list_count(list), new_user->user->session_id);
        return ADD_USER_SUCCESS;
    }
    else
        fprintf(stderr, "Failed to allocate memory for new user\n");
    return ADD_USER_FAILURE;
}

void user_list_remove_by_name(user_list_t *list, char * name){
    if (name == NULL) {
        fprintf(stderr, "Name is NULL\n");
        return;
    }
    link_user_t * current = list->head;
    link_user_t * previous = NULL;
    while (current) {
        if (strcmp((char *) current->user->username, name) == 0) {
            fprintf(stdout, "User %s has been removed to user list\n", name);
            fprintf(stdout, "Now there is %d people in session \"%s\"\n", user_list_count(list),current->user->session_id);
            if (previous) {
                previous->next = current->next;
            } else {
                list->head = current->next;
            }
            free(current->user);
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void user_list_remove_all(user_list_t *list){
    link_user_t * current = list->head;
    link_user_t * next = NULL;
    while (current) {
        next = current->next;
        free(current->user);
        free(current);
        current = next;
    }
    free(list);
}

void user_list_print(user_list_t *list){
    link_user_t * current = list->head;
    while (current) {
        fprintf(stdout,"%s\n", current->user->username);
        fprintf(stdout,"%s\n", current->user->session_id);
        fprintf(stdout,"%d\n", current->user->status);
        if (current->user->role == ADMIN) {
            fprintf(stdout,"Admin\n");
        } else {
            fprintf(stdout,"User\n");
        }
        current = current->next;
    }
}
user_t *user_list_find(user_list_t *list, unsigned char *username){
    if (username == NULL) {
        fprintf(stderr, "Username is NULL\n");
        return NULL;
    }
    link_user_t * current = list->head;
    while (current) {
        if (strcmp((char *) current->user->username, (char *) username) == 0) {
            return current->user;
        }
        current = current->next;
    }
    return NULL;
}

int user_list_count(user_list_t *list){
    int count = 0;
    link_user_t * current = list->head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}
int user_list_count_by_session_id(user_list_t *list, char *session_id){
    if (session_id == NULL) {
        fprintf(stderr, "Session ID is NULL\n");
        return 0;
    }
    if (strcmp((char *) session_id, NOT_IN_SESSION) == 0) {
        fprintf(stderr, "session does not exist\n");
        return 0;
    }
    int count = 0;
    link_user_t * current = list->head;
    while (current) {
        if (strcmp((char *) current->user->session_id, (char *) session_id) == 0) {
            count++;
        }
        current = current->next;
    }
    return count;
}

user_t *return_user_by_username(user_list_t *list, unsigned char *username){
    if (username == NULL) {
        fprintf(stderr, "Username is NULL\n");
        return NULL;
    }
    link_user_t * current = list->head;
    while (current) {
        if (strcmp((char *) current->user->username, (char *) username) == 0) {
            return current->user;
        }
        current = current->next;
    }
    return NULL;
}

int create_user(user_t * user, unsigned char * username, unsigned char * password,
                unsigned char status, char * session_id, char * ip_address, unsigned int port, int socket_fd, char role){
    strcpy((char *) user->username, (char*)username);
    strcpy((char *) user->password, (char*)password);
    if (session_id == NULL) {
        strcpy((char *) user->session_id, NOT_IN_SESSION);
    } else {
        strcpy((char *) user->session_id, session_id);
    }
    if (ip_address == NULL) {
        strcpy((char *) user->ip_address, IP_NOT_FOUND);
    } else {
        strcpy((char *) user->ip_address, ip_address);
    }
    user->status = status;
    user->port = port;
    user->socket_fd = socket_fd;
    user->role = role;
    return USER_CREATE_SUCCESS;
}