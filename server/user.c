#include "user.h"

user_database* user_list_init(){
    int data_base_initial_capacity = 16;
    user_database* user_list = malloc(sizeof(user_database));
    user_list->capacity = data_base_initial_capacity;
    user_list->count = 0;
    user_list->user_list = malloc(sizeof(user_t) * user_list->capacity);
    if (user_list == NULL) {
        fprintf(stderr, "Failed to allocate memory for user list\n");
        return NULL;
    }
    return user_list;
}

int create_user(user_t * user, user_t * user_to_add) {
    strcpy((char*) user->username,(char*) user_to_add->username);
    strcpy((char*) user->password,(char*) user_to_add->password);
    strcpy(user->session_id, user_to_add->session_id);
    strcpy(user->ip_address, user_to_add->ip_address);
    user->status = user_to_add->status;
    user->port = user_to_add->port;
    user->socket_fd = user_to_add->socket_fd;
    user->role = user_to_add->role;
    return USER_CREATE_SUCCESS;
}

int user_list_add_user(user_database* list, user_t* user_data, pthread_mutex_t * mutex) {
    pthread_mutex_t **mutex_ptr = &mutex;
    int user_count = user_list_count(list, *mutex_ptr);
    pthread_mutex_lock(mutex);
    if (user_count >= MAX_USER_COUNT) {
        fprintf(stderr, "User list is full\n");
        pthread_mutex_unlock(mutex);
        return USER_LIST_FULL;
    }
    if (user_list_find(list, user_data->username) != NULL) {
        fprintf(stderr, "User already exists\n");
        pthread_mutex_unlock(mutex);
        return USER_ALREADY_EXIST;
    }
    user_t new_user = {0};
    create_user(&new_user, user_data);

    list->user_list[list->count] = new_user;
    list->count++;
    fprintf(stdout, "User %s added to user list\n", list->user_list[list->count-1].username);
    pthread_mutex_unlock(mutex);
    return ADD_USER_SUCCESS;
}

//will never be called
void user_list_remove_by_name(user_database* list, char * name, pthread_mutex_t * mutex) {
    pthread_mutex_t **mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    for (int i = 0; i < list->count; ++i) {
        if (strcmp((char*) list->user_list[i].username, name) == 0) {
            fprintf(stdout, "User %s removed from user list\n", list->user_list[i].username);
            memset(&list->user_list[i], 0, sizeof(user_t));
            for (int j = i; j < list->count - 1; ++j) {
                list->user_list[j] = list->user_list[j + 1];
            }
            list->count--;
            pthread_mutex_unlock(mutex);
            return;
        }
    }
    fprintf(stderr, "User %s not found\n", name);
    pthread_mutex_unlock(*mutex_ptr);
}

void user_list_remove_all(user_database* list, pthread_mutex_t * mutex){
    pthread_mutex_t **mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    free(list->user_list);
    free(list);
    pthread_mutex_unlock(*mutex_ptr);
}

void user_list_print(user_database* list){
    fprintf(stdout, "User list:\n");
    fprintf(stdout, "%*s %*s %*s %*s\n", -MAX_NAME, "Name", -MAX_SESSION_LENGTH, "Session ID", -INET6_ADDRSTRLEN, "IP Address", -6, "Port");
    for (int i = 0; i < list->count; ++i) {
        fprintf(stdout, "%*s %*s %*s %*d\n", -MAX_NAME, list->user_list[i].username,
                -MAX_SESSION_LENGTH,list->user_list[i].session_id, -INET6_ADDRSTRLEN, list->user_list[i].ip_address, -6, list->user_list[i].port);
    }
}

void active_user_list_print(user_database * list){
    fprintf(stdout, "Active user list:\n");
    fprintf(stdout, "%*s %*s\n", -MAX_NAME, "Name", -MAX_SESSION_LENGTH, "Session ID");
    for (int i = 0; i < list->count; ++i) {
        if (list->user_list[i].status == ONLINE) {
            fprintf(stdout, "%*s %*s\n", -MAX_NAME, list->user_list[i].username,
                    -MAX_SESSION_LENGTH, list->user_list[i].session_id);
        }
    }
}

void get_active_user_list(user_database* list, char * active_user_list,  pthread_mutex_t *mutex) {
    if (list == NULL){
        fprintf(stderr, "Error: list is NULL.\n");
        return;
    }
    if(active_user_list == NULL){
        fprintf(stderr, "parameter empty!.\n");
        return;
    }
    strcat(active_user_list, "Active User List:\n");
    char message[maximum_buffer_size];
    sprintf(message, "%*s %*s\n", -MAX_NAME, "Username", -MAX_SESSION_COUNT, "Session ID");
    strcat(active_user_list, message);

    for (int i = 0; i < list->count; ++i) {
        if (list->user_list[i].status == ONLINE) {
            memset(message, 0, sizeof(message));
            sprintf(message, "%*s %*s\n", -MAX_NAME, list->user_list[i].username, -MAX_SESSION_COUNT, list->user_list[i].session_id);
            strcat(active_user_list, message);
        }
    }
    fprintf(stdout, "%s", active_user_list);

}
user_t * user_list_find(user_database* list, unsigned char * username) {
    if (username == NULL){
        fprintf(stderr, "Error: username is NULL.\n");
        return NULL;
    }
    for (int i = 0; i < list->count; ++i) {
        if (strcmp((char*) list->user_list[i].username, (char*) username) == 0) {
            return &list->user_list[i];
        }
    }
    return NULL;
}

int user_list_count(user_database* list, pthread_mutex_t * mutex){
    pthread_mutex_t **mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    unsigned int count = list->count;
    pthread_mutex_unlock(*mutex_ptr);
    return (int)count;
}

int user_list_count_by_session_id(user_database* list, char * session_id, pthread_mutex_t * mutex){
    pthread_mutex_t **mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    if (strcmp(session_id, NOT_IN_SESSION) == 0){
        fprintf(stderr, "Error: session_id reserved.\n");
        return SESSION_NAME_RESERVED;
    }
    int count = 0;
    for (int i = 0; i < list->count; ++i) {
        if (strcmp(list->user_list[i].session_id, session_id) == 0) {
            count++;
        }
    }
    pthread_mutex_unlock(*mutex_ptr);
    return count;
}

user_t* return_user_by_username(user_database* list, unsigned char * username){
    if (username == NULL){
        fprintf(stderr, "Error: username is NULL.\n");
        return NULL;
    }
    for (int i = 0; i < list->count; ++i) {
        if (strcmp((char*) list->user_list[i].username, (char*) username) == 0) {
            return &list->user_list[i];
        }
    }
    return NULL;
}

void user_exit_current_session(user_database* list, char * username, pthread_mutex_t * mutex){
    pthread_mutex_lock(mutex);
    user_t * user = return_user_by_username(list, (unsigned char *) username);
    char session_id[MAX_SESSION_LENGTH];
    strcpy(session_id, user->session_id);
    user->in_session = OFFLINE;
    user->role = USER;
    memset(user->session_id, 0, sizeof(user->session_id));
    strcpy(user->session_id, NOT_IN_SESSION);
    pthread_mutex_unlock(mutex);
    if (user_list_count_by_session_id(list, session_id, mutex) == 0){
        fprintf(stdout, "Session %s is empty and has been dismissed!\n", user->session_id);
    }

}
