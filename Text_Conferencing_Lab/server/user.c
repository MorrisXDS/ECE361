#include "user.h"

/* Initialize a user list */
user_database* user_database_init() {
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

/* Create a user */
int create_user(user_t* user, user_t* user_to_add) {
    strcpy((char*)user->username, (char*)user_to_add->username);
    strcpy((char*)user->password, (char*)user_to_add->password);
    strcpy(user->session_id, user_to_add->session_id);
    strcpy(user->ip_address, user_to_add->ip_address);
    user->status = user_to_add->status;
    user->port = user_to_add->port;
    user->socket_fd = user_to_add->socket_fd;
    user->role = user_to_add->role;
    return USER_CREATE_SUCCESS;
}

/* add a user to a user_database
 * and do error check*/
int user_database_add_user(user_database* list, user_t* user_data, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    int user_count = get_user_count_in_database(list, *mutex_ptr);
    pthread_mutex_lock(mutex);
    //check if user list is full
    if (user_count >= MAX_USER_COUNT) {
        fprintf(stderr, "User list is full\n");
        pthread_mutex_unlock(mutex);
        return USER_LIST_FULL;
    }
    //check if user already exists
    if (database_search_user(list, user_data->username) != NULL) {
        fprintf(stderr, "User already exists\n");
        pthread_mutex_unlock(mutex);
        return USER_ALREADY_EXIST;
    }
    //add user to database
    user_t new_user = { 0 };
    create_user(&new_user, user_data);

    list->user_list[list->count] = new_user;
    list->count++;
    fprintf(stdout, "User %s added to user list\n", list->user_list[list->count - 1].username);
    pthread_mutex_unlock(mutex);
    return ADD_USER_SUCCESS;
}

//will never be called
void remove_user_by_name_in_database(user_database* list, char* name, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    for (int i = 0; i < list->count; ++i) {
        if (strcmp((char*)list->user_list[i].username, name) == 0) {
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

/* Remove all users from a database
 * and releases resources held
 * by the database*/
void remove_all_users_in_database(user_database* list, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    free(list->user_list);
    free(list);
    pthread_mutex_unlock(*mutex_ptr);
}

/* Print all users in a database */
void print_all_users_in_a_database(user_database* list) {
    fprintf(stdout, "User list:\n");
    fprintf(stdout, "%*s %*s %*s %*s\n", -MAX_NAME, "Name", -MAX_SESSION_LENGTH, "Session ID", -INET6_ADDRSTRLEN, "IP Address", -6, "Port");
    for (int i = 0; i < list->count; ++i) {
        fprintf(stdout, "%*s %*s %*s %*d\n", -MAX_NAME, list->user_list[i].username,
            -MAX_SESSION_LENGTH, list->user_list[i].session_id, -INET6_ADDRSTRLEN, list->user_list[i].ip_address, -6, list->user_list[i].port);
    }
}

/* print all online users in a database */
void print_active_users_in_a_database(user_database* list) {
    fprintf(stdout, "Active user list:\n");
    fprintf(stdout, "%*s %*s\n", -MAX_NAME, "Name", -MAX_SESSION_LENGTH, "Session ID");
    for (int i = 0; i < list->count; ++i) {
        if (list->user_list[i].status == ONLINE) {
            fprintf(stdout, "%*s %*s\n", -MAX_NAME, list->user_list[i].username,
                -MAX_SESSION_LENGTH, list->user_list[i].session_id);
        }
    }
}

/* print all online users in a session by session_id
 * format: <username> <session_id> <role>*/
int get_active_user_list_in_a_session(user_database* list, char* active_user_list, char* session_id, pthread_mutex_t* mutex) {
    if (get_user_count_in_a_session(list, session_id, mutex) == 0) {
        fprintf(stderr, "Error: session_id is invalid.\n");
        return SESSION_NAME_INVALID;
    }
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    strcat(active_user_list, "Active User List:\n");
    char message[maximum_buffer_size];
    sprintf(message, "%*s %*s %*s\n", -MAX_NAME, "Username", -MAX_SESSION_COUNT, "Session ID", -6, "Role");
    strcat(active_user_list, message);
    char role[6];

    for (int i = 0; i < list->count; ++i) {
        if (strcmp(list->user_list[i].session_id, session_id) == 0) {
            memset(role, 0, 5);
            if (list->user_list[i].role == ADMIN) {
                strcpy(role, "ADMIN");
            }
            else {
                strcpy(role, "USER");
            }
            memset(message, 0, maximum_buffer_size);
            sprintf(message, "%*s %*s %*s\n", -MAX_NAME, list->user_list[i].username,
                -MAX_SESSION_COUNT, list->user_list[i].session_id, -6, role);
            strcat(active_user_list, message);
        }
    }
    pthread_mutex_unlock(*mutex_ptr);
    return 1;
}

void set_the_only_user_admin(user_database* list, char* session_id, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    for (int i = 0; i < list->count; ++i) {
        if (strcmp(list->user_list[i].session_id, session_id) == 0) {
            if (list->user_list[i].role == USER)
                list->user_list[i].role = ADMIN;
        }
    }
    pthread_mutex_unlock(*mutex_ptr);
}

/* print all online users in a database
 * format: <username> <session_id>*/
void get_active_user_list_in_database(user_database* list, char* active_user_list, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    if (list == NULL) {
        fprintf(stderr, "Error: list is NULL.\n");
        return;
    }
    if (active_user_list == NULL) {
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
    pthread_mutex_unlock(*mutex_ptr);
}

/* determine if a user exists in a database
 * and return a pointer to the user_t
 * (which may be NULL)*/
user_t* database_search_user(user_database* list, unsigned char* username) {
    if (username == NULL) {
        fprintf(stderr, "Error: username is NULL.\n");
        return NULL;
    }
    for (int i = 0; i < list->count; ++i) {
        if (strcmp((char*)list->user_list[i].username, (char*)username) == 0) {
            return &list->user_list[i];
        }
    }
    return NULL;
}

/* return the number of users in a database */
int get_user_count_in_database(user_database* list, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    unsigned int count = list->count;
    pthread_mutex_unlock(*mutex_ptr);
    return (int)count;
}

/* return the number of users in a session */
int get_user_count_in_a_session(user_database* list, char* session_id, pthread_mutex_t* mutex) {
    pthread_mutex_t** mutex_ptr = &mutex;
    pthread_mutex_lock(*mutex_ptr);
    int count = 0;
    for (int i = 0; i < list->count; ++i) {
        if (strcmp(list->user_list[i].session_id, session_id) == 0) {
            count++;
        }
    }
    pthread_mutex_unlock(*mutex_ptr);
    fprintf(stdout, "the user count in session %s is %d\n", session_id, count);
    return count;
}

/* return a pointer to the user_t by username
 * if they exist otherwise return NULL*/
user_t* return_user_by_username(user_database* list, unsigned char* username) {
    if (username == NULL) {
        fprintf(stderr, "Error: username is NULL.\n");
        return NULL;
    }
    for (int i = 0; i < list->count; ++i) {
        if (strcmp((char*)list->user_list[i].username, (char*)username) == 0) {
            return &list->user_list[i];
        }
    }
    return NULL;
}

/* remove a user from a session if they
 * are in a session*/
void user_exit_current_session(user_database* list, char* username, pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex);
    user_t* user = return_user_by_username(list, (unsigned char*)username);
    char session_id[MAX_SESSION_LENGTH];
    strcpy(session_id, user->session_id);
    if (strcmp(user->session_id, NOT_IN_SESSION) == 0) {
        return;
    }
    user->in_session = OFFLINE;
    user->role = USER;
    memset(user->session_id, 0, sizeof(user->session_id));
    strcpy(user->session_id, NOT_IN_SESSION);
    pthread_mutex_unlock(mutex);
    if (get_user_count_in_a_session(list, session_id, mutex) == 0) {
        fprintf(stdout, "Session %s is empty and has been dismissed!\n", session_id);
    }
    if (get_user_count_in_a_session(list, session_id, mutex) == 1) {
        fprintf(stdout, "there is only one user in the session\n");
        set_the_only_user_admin(list, session_id, mutex);
    }

}
