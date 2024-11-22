/*
 * echo-server.c
 */

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

#define BACKLOG 10
#define BUF_SIZE 4096

struct client {
    char * username;
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
        cl->number_clients += 1;
        if (cl->number_clients % 10 == 0) {
            // Number of clients is divisible by 10, so we need to allocate more space.
            // This works becasue we de allocation 10 clients at a time.
            cl->clients = realloc(cl->clients, cl->number_clients + 10);
        }
        /* announce our communication partner */
       
        current_record->remote_ip = inet_ntoa(remote_sa.sin_addr);
        current_record->remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", current_record->remote_ip, current_record->remote_port);
        pthread_t child_thread;
        pthread_create(&child_thread, NULL, client_instance, current_record);

        // Close needs to be called in the new thread!
    }

}

void * client_instance(void* data) {
    struct client *c = data;
    char buf[BUF_SIZE];
    int bytes_received;
    while(1) {
        /* receive and echo data until the other end closes the connection */
        while((bytes_received = recv(c->fd, buf, BUF_SIZE, 0)) > 0) {
            printf("Message Recieved from %d\n", c->remote_port);
            
            for (int i = 0; i < (c->client_list->number_clients); i++){
                struct client *loop_client = c->client_list->clients + i;
                printf("Sending Message to %d\n", loop_client->remote_port);
                fflush(stdout);
                send(loop_client->fd, buf, bytes_received,0);
            }
            /* send it back */
            //send(c->fd, buf, bytes_received, 0);
        }
        printf("\n");

        close(c->fd);
        return NULL;
    }
}

