#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <gtk/gtk.h>

GtkWidget *text_view;
GtkWidget *entry;
GtkWidget *username_entry;

#define serverPort 5678
#define maxMessageSize 281
#define Hostname "localhost"
#define SA struct sockaddr

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;

char username[11];
int clientSocket;
bool open = true;
char message[maxMessageSize];
bool sendFlag=false;

//This function opens a connection from the client to the server at hostname: port
int open_clientfd(char *hostname, int port) {

    //Create Socket
    int clientfd; //Socket Descriptor
    struct hostent *hp;
    struct sockaddr_in serveraddr;
   
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error creating socket");
        return -1;
    }
   
    // Create Address 
    // Fill in the server's IP address and port
    if ((hp = gethostbyname(hostname)) == NULL){
        herror("Error getting host by name");
        return -2;
    }
   
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    bcopy((char *)hp->h_addr_list[0],(char *)&serveraddr.sin_addr.s_addr, hp->h_length);

    //Establish a connection with the server 
    if (connect(clientfd, (SA *) &serveraddr,sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}



void * receiveMessages(void * x){
    char messageReceived[maxMessageSize];
    while(open){
        memset(messageReceived, '\0', strlen(messageReceived));          
        recv(clientSocket, messageReceived, maxMessageSize, 0);
        printf("%s\n" , messageReceived);
        pthread_mutex_lock(&lck);
        //Append the new message
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gchar* msgr=g_strdup(messageReceived);
        strcpy(msgr,messageReceived);
        gtk_text_buffer_insert_at_cursor(buffer, msgr, -1);
        //Append a newline after the message
        gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
        pthread_mutex_unlock(&lck);
        g_free(msgr);

    }
    pthread_exit(NULL);
}

void buttonclick(){
    pthread_mutex_lock(&lck);
    const gchar *text=gtk_entry_get_text(GTK_ENTRY(entry));
    strcpy(message,text);
    sendFlag=true;
    gtk_entry_set_text(GTK_ENTRY(entry), "");
    pthread_mutex_unlock(&lck);
}

void * sendMessages(void * y){  
    
    size_t len = strlen(message);
    char messagewithYou[maxMessageSize+5];
    char quit[maxMessageSize]="!q";
    char temp[11];
    memset(temp, '\0', strlen(temp)); 
    memset(username, '\0', strlen(username));
    //waiting for username entry

    while(strcmp(username,temp)==0);
    size_t size = strlen(username);
    if(size > 0 && username[size-1]=='\n'){
    	username[size-1] = '\0';
    }
    //Sending username to server
    send(clientSocket, username, strlen(username), 0);
    printf("%s\n",username);
    while(1){
        if (sendFlag==true){
            send(clientSocket, message, strlen(message),0);
            pthread_mutex_lock(&lck);
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            memset(messagewithYou, '\0', strlen(messagewithYou));
            strcat(messagewithYou,"You: ");
            strcat(messagewithYou,message);
            gchar* msg=g_strdup(messagewithYou);                        
            gtk_text_buffer_insert_at_cursor(buffer, msg, -1);

            // Append a newline after the message
            gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
            pthread_mutex_unlock(&lck);
            g_free(msg);
            
            if(strcmp(quit,message)==0){break;}
            memset(message, '\0', strlen(message));         
            size_t len = strlen(message);
            
            sendFlag=false;
  	} 
    }
     open = false;
     pthread_exit(NULL);
}

static void on_destroy(GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_main_quit();
    char quit[281]="!q";
    send(clientSocket, quit, strlen(quit),0);
    close(clientSocket);
    exit(0);
}

void switch_to_chat(GtkButton *button, GtkStack *stack) {
    // Get the stack from the button's parent container
    stack = GTK_STACK(gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(button))));
    const gchar *txt=gtk_entry_get_text(GTK_ENTRY(username_entry));
    strcpy(username,txt);
    // Switch to the chat page
    gtk_stack_set_visible_child_name(stack, "chat_page");
}

int main(int argc, char* argv[]){
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Group Chat");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 400, 300);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_destroy), NULL);
    
    GtkWidget *stack = gtk_stack_new();
    gtk_container_add(GTK_CONTAINER(window), stack);
    
    // Create the login page
    GtkWidget *login_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_stack_add_named(GTK_STACK(stack), login_page, "login_page");

    username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Enter username (up to 10 characters)");
    

    GtkWidget *login_button = gtk_button_new_with_label("Login");
    g_signal_connect(G_OBJECT(login_button), "clicked", G_CALLBACK(switch_to_chat), stack);

    gtk_box_pack_start(GTK_BOX(login_page), username_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(login_page), login_button, FALSE, FALSE, 0);

    // Create a scrolled window to contain the text view
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    // Create the text view
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Create an entry widget for typing messages
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type your message...");

    // Create a button to send messages
    GtkWidget *send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(buttonclick), NULL);


    // Create a vertical box to arrange widgets
    GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
    
    gtk_stack_add_named(GTK_STACK(stack), vbox, "chat_page");
    
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);

    // Add the vertical box to the main window
    //gtk_container_add(GTK_CONTAINER(window), vbox);

    // Show all widgets
    gtk_widget_show_all(window);


    //creating the client socket using TCP protocol
    //& connecting the client socket to the server socket
    clientSocket = open_clientfd(Hostname, serverPort);

    
    //creating thread for sending messages to the client
    pthread_t sendingThread;    
    pthread_create(&sendingThread, NULL, sendMessages, NULL);
   

    //creating thread for receiving messages from server
    pthread_t receivingThread;  
    pthread_create(&receivingThread, NULL, receiveMessages, NULL);
    // Start the GTK main loop
    gtk_main();
    
    pthread_join (sendingThread, NULL);
    pthread_join (receivingThread, NULL);
    
    close(clientSocket);
    exit(0);
}
