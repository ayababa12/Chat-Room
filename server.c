#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define max_clients 10
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t ids[max_clients];
struct sockaddr_in client_addresses[max_clients];
int client_sockets[max_clients];
int client_id[max_clients];

//This function returns a listening port
int open_listenfd(int port) 
{ 
    int listenfd, optval=1; 
    struct sockaddr_in serveraddr; 
    /* Create a socket descriptor */ 
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1; 
        }
    /* Eliminates "Address already in use" error from bind. */ 
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int)) < 0){
        return -1;
        }
    bzero((char *) &serveraddr, sizeof(serveraddr)); 
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0){
        return -1; 
        }
    /* Make it a listening socket ready to accept connection requests */ 
    if (listen(listenfd, max_clients) < 0){
        return -1;
        }
    return listenfd; 
}


void *clientSession(void* param){
    int i=* (int*) param;
    char message[281];
    char prevmessage[281];
    char username[11];
    memset(username, 0, sizeof(username)); 
    
    char messageWithUsername[291]; //the message that will be broadcast to all clients, it is in the form:   <username>:<message>
    recv(client_sockets[i],username, (sizeof(username))-1, 0); //the client must first enter his username
    
    //printf("%s\n",username);
    while(1){
    	memset(prevmessage, '\0', strlen(prevmessage));   	
        strcpy(prevmessage,message);
        memset(message, '\0', strlen(message));
        recv(client_sockets[i],message, (sizeof(char))*280, 0);
        char quit[281]="!q";
        if (strcmp(quit,message)==0){ //the client exits the chat by typing !q
            close(client_sockets[i]);
            client_sockets[i]=0;
            pthread_exit(NULL);
        }
        memset(messageWithUsername, '\0', strlen(messageWithUsername)); 
        strcpy(messageWithUsername,username);
        strcat(messageWithUsername,": ");
        strcat(messageWithUsername,message);
        if (strcmp(prevmessage,message)!=0){
            pthread_mutex_lock(&mutex); //CRITICAL SECTION
            for(int j=0;j<max_clients;j++){
                if(client_sockets[j]!=0 && j!=i){
                    send(client_sockets[j],messageWithUsername,strlen(messageWithUsername),0);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }
}


int main(){
    int listen_fd=open_listenfd(5678); //assigned PORT NUMBER 5678
    struct sockaddr_in address;
    socklen_t addrlen=sizeof(struct sockaddr_in);
    int i=0;
    while(1){
        if ((client_sockets[i]= accept(listen_fd, (struct sockaddr*)&client_addresses[i],&addrlen))< 0) {
            close(listen_fd);
            return -2;
        }
        addrlen=sizeof(struct sockaddr_in);
        client_id[i]=i; //to pass the index "i" safely to each thread. This index will be used to obtain client_sockets[i]...
        pthread_create(&ids[i],NULL,clientSession,&client_id[i]);
        i++;

        //if maximum number of clients is reached: block main thread until the group chat is empty again
        if (i==max_clients){
            i=0;
            while (i<max_clients){//waits for all threads to exit
                pthread_join(ids[i],NULL);
                i++;
            }
            i=0;
        }
    }
}

