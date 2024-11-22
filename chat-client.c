#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]){
    if (argc != 3) {
        return -1;
    }
    char *dest_hostname = argv[1];
    char *dest_port = argv[2];
    int rc;
    char buf[BUF_SIZE];
    int n;
    struct addrinfo hints, *res;

    // Open the Socket:
    int conn_fd = socket(PF_INET, SOCK_STREAM, 0);

    // Set up hints (must start by setting everything to 0)
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /*
    DNS LOOKUP!
    &hints: contains what type of address we want to look up
    &res: At end, contains the ip adress for the hostname and port
    */
    if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    // Connect to the server:
    if(connect(conn_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        exit(2);
    }
    puts("Connected\n");
    int n2;
    // Read data from terminal and send to server
    while((n = read(0, buf, BUF_SIZE)) >= 0) {
        send(conn_fd, buf, n, 0);
        if ((n2 = recv(conn_fd, buf, BUF_SIZE, 0)) > 0){
            puts(buf);
        }
    }
    close(conn_fd);
}