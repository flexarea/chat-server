#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stddef.h>

#define BACKLOG 10
#define BUF_SIZE 4096

struct client {
    char * username;
    int name_length;
    int fd;
    uint16_t remote_port;
    char *remote_ip;
    struct client_list *client_list;
};
struct client_list {
    struct client * clients;
    int number_clients;
};

void *client_instance(void* data);
int send_to_all_clients(struct client * c, char* s, int l);

int main(int argc, char *argv[])
{
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    int rc;
    struct sockaddr_in remote_sa;
    socklen_t addrlen;
    
    struct client_list *cl = malloc(sizeof(struct client_list));
    cl->clients = malloc(sizeof(struct client) * 10); // Set arbitrarily to allow 10 users before allocating more
    cl->number_clients = 0;


    listen_port = argv[1];

    /* create a socket */
    listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    /* bind it to a port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    bind(listen_fd, res->ai_addr, res->ai_addrlen);

    /* start listening */
    listen(listen_fd, BACKLOG);

    /* infinite loop of accepting new connections and handling them */
    while(1) {
        /* accept a new connection (will block until one appears) */
        addrlen = sizeof(remote_sa);
        conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);
        // Once it is done this iteration of the loop, it will be ready to accept again.
        
        // Update the number of active clients
        struct client *current_record = cl->clients + cl->number_clients;
        current_record->fd = conn_fd;
        current_record->client_list = cl;
        current_record->username  = "User unkown";
        current_record->name_length = 11;

        cl->number_clients += 1;
        if (cl->number_clients % 10 == 0) {
            // Number of clients is divisible by 10, so we need to allocate more space.
            // This works becasue we de allocation 10 clients at a time.
            cl->clients = realloc(cl->clients, cl->number_clients + 10);
        }
        /* announce our communication partner */
       
        current_record->remote_ip = inet_ntoa(remote_sa.sin_addr);
        current_record->remote_port = ntohs(remote_sa.sin_port);
        printf("New connection from %s:%d\n", current_record->remote_ip, current_record->remote_port);
        pthread_t child_thread;
        pthread_create(&child_thread, NULL, client_instance, current_record);

        // Close needs to be called in the new thread!
    }

}

void * client_instance(void* data) {
    struct client *c = data;
    
    int bytes_received;
    while(1) {
        char in_buff[BUF_SIZE] = {'\0'};
        while((bytes_received = recv(c->fd, in_buff, BUF_SIZE, 0)) > 0) {
            //printf("Message Recieved from %d\n", c->remote_port);
            int bytes_to_send = 0;
            char out_buff[BUF_SIZE] = {'\0'};
            time_t t;
            time(&t);
            struct tm * timeinfo;
            // man 3 localtime is a magical thing
            timeinfo = localtime( &t );
            if (in_buff[0] == '/'){
                // we may be in the /nick case, check for "nick " (trailing space intentional)
                char command_buff[6] = {'\0'};
                strncpy(command_buff,in_buff+1,5);
                if (strcmp(command_buff, "nick ") == 0) {
                    char * name_buff = malloc(BUF_SIZE);
                    int name_length = stpncpy(name_buff,(in_buff+6),BUF_SIZE) - name_buff;
                    name_buff = realloc(name_buff, name_length);
                    name_buff[name_length-1] = '\0';
                    // 78 is total size of string without name
                    bytes_to_send = snprintf(out_buff, 78 + name_length, "%02d:%02d:%02d: User unknown (%s:%d) is now known as %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, c->remote_ip, c->remote_port, name_buff);
                    
                    c->username = name_buff;
                    c->name_length = name_length;
                }else{
                    continue;
                }
            }else{
                bytes_to_send = snprintf(out_buff, bytes_received + 12 + c->name_length, "%02d:%02d:%02d: %s: %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, c->username ,in_buff);
            }
            send_to_all_clients(c, out_buff, bytes_to_send);
        }
        printf("\n");
        close(c->fd);
        return NULL;
    }
}

int send_to_all_clients(struct client * c, char* s, int len) {
    for (int i = 0; i < (c->client_list->number_clients); i++){
                struct client *loop_client = c->client_list->clients + i;
                // ADD 10 bytes to account for the time
                send(loop_client->fd, s, len,0);
    }
    return 1;
}

