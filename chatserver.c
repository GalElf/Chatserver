#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

/*======= declaration of define =======*/

#define forEver() for (;;)
#define MAX_SIZE 1024

/*======= declaration of function =======*/

void checkIfStringIsValidNumber(char *str);
void checkArgvValidtion(int argc, char *argv[]);
void sendErrorMessage();
int createServer(int port);
void sendServerError(char *msg, int sockfd);
void createChat(int sockfd, int max_clients);
int findMaxFd(int *arr, int max_client, int sockfd);
void closeClientFds(int *clientsFds, int max_clients);

/*======= Main =======*/

int main(int argc, char *argv[])
{
    checkArgvValidtion(argc, argv); // check if the input in the argv and argc is valid

    int port = atoi(argv[1]);
    int max_clients = atoi(argv[2]);

    // create the main socket for the chat server
    int sockfd = createServer(port);

    // create the chat server to connect between the clients to server
    createChat(sockfd, max_clients);

    // free all allocation memory and close all open socket
    close(sockfd);
    return 0;
}

// this function create the chat server to connect between the clients to server
void createChat(int sockfd, int max_clients)
{
    // create arrays of fd for the clients
    int clientsFds[max_clients];
    // initialize the array with -1
    for (int i = 0; i < max_clients; i++)
    {
        clientsFds[i] = -1;
    }

    int maxfd;
    int countFds = 0;
    char buffer[MAX_SIZE];
    int nbytes = 0;
    // create the set
    fd_set readfd;
    fd_set writefd;
    FD_ZERO(&readfd);
    FD_ZERO(&writefd);
    // create the set copy
    fd_set copyReadfd;
    fd_set copyWritefd;
    FD_ZERO(&copyReadfd);
    FD_ZERO(&copyWritefd);

    FD_SET(sockfd, &readfd); // set the welcome socket in the readfd
    // infinite loop
    forEver()
    {
        // initialize the copy
        copyReadfd = readfd;
        copyWritefd = writefd;
        maxfd = findMaxFd(clientsFds, max_clients, sockfd); // find the max value from the all the open socket
        // use select
        if (select(maxfd, &copyReadfd, &copyWritefd, NULL, NULL) < 0)
        {
            perror("Error, select has failed");
            closeClientFds(clientsFds, max_clients);
            free(clientsFds);
            close(sockfd);
        }

        if (FD_ISSET(sockfd, &copyReadfd))
        { // if its ready to read and its the welcome socket
            for (countFds = 0; countFds < max_clients; countFds++)
            {
                if (clientsFds[countFds] == -1)
                { // accept new client
                    clientsFds[countFds] = accept(sockfd, NULL, NULL);
                    FD_SET(clientsFds[countFds], &readfd);
                    FD_SET(clientsFds[countFds], &writefd);
                    break;
                }
            }
            if (countFds == max_clients)
            { // if we have reach to the max client unset the welcome socket
                FD_CLR(sockfd, &readfd);
            }
        }

        for (int i = 0; i < max_clients; i++)
        {
            if (FD_ISSET(clientsFds[i], &copyReadfd))
            { // if its ready to read and isn't the welcome socket
                printf("fd %d is ready to read\n", clientsFds[i]);
                nbytes = read(clientsFds[i], buffer, MAX_SIZE - 1); // read from client into ab buffer
                if (nbytes > 0)
                {
                    for (int j = 0; j < max_clients; j++)
                    {
                        if (FD_ISSET(clientsFds[j], &copyWritefd))
                        { // if its ready to write and isn't the welcome socket
                            printf("fd %d is ready to write\n", clientsFds[j]);
                            write(clientsFds[j], buffer, nbytes); // write from the buffer into all the active socket
                        }
                    }
                }
                else if (nbytes == 0)
                { // if there is nothing to read
                    FD_CLR(clientsFds[i], &readfd);
                    FD_CLR(clientsFds[i], &writefd);
                    close(clientsFds[i]);
                    clientsFds[i] = -1;
                    FD_SET(sockfd, &readfd);
                }
                else
                { // nbytes < 0
                    perror("Error, read from client has failed");
                    closeClientFds(clientsFds, max_clients);
                    free(clientsFds);
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    // close all the open client fds
    closeClientFds(clientsFds, max_clients);
}

// this function check if the user insert valid input
void checkArgvValidtion(int argc, char *argv[])
{
    if (argc != 3)
    {
        sendErrorMessage("Error, there is to many or less argumentsin in the argv\n");
    }
    checkIfStringIsValidNumber(argv[1]);
    checkIfStringIsValidNumber(argv[2]);
    // check if the number is out of bounds
    if (atoi(argv[1]) < 1 || atoi(argv[1]) > 65535 || atoi(argv[2]) < 1)
    {
        sendErrorMessage("Error, the port or max_clients are out of of\n");
    }
}

// this function get string and check if he contain only number any char that is not 0 to 9 there will be  USAGE ERROR
void checkIfStringIsValidNumber(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            sendErrorMessage("Error, the port or max_clients are invalid\n");
        }
    }
}

// print error message if one of the value invalid
void sendErrorMessage(char *msg)
{
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

// create the main socket
int createServer(int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR, sockfd has failed");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    // create bind
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        sendServerError("ERROR, bind has failed", sockfd);
    }
    // create listen
    if (listen(sockfd, 5) < 0)
    {
        sendServerError("ERROR, listen has failed", sockfd);
    }
    return sockfd;
}

// send an error message that happen while creating the socket
void sendServerError(char *msg, int sockfd)
{
    perror(msg);
    close(sockfd);
    exit(EXIT_FAILURE);
}

// this function find the max value of the fs and return him with +1
int findMaxFd(int *arr, int max_client, int sockfd)
{
    int max = sockfd;
    for (int i = 0; i < max_client; i++)
    {
        if (max < arr[i])
        {
            max = arr[i];
        }
    }
    return max + 1;
}

// close all the client socket
void closeClientFds(int *clientsFds, int max_clients)
{
    for (int i = 0; i < max_clients; i++)
    {
        if (clientsFds[i] != -1)
        {
            close(clientsFds[i]);
        }
    }
}