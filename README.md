# Group Chat Room

## Instructions for running the code
1. please install GTK3 using the following command: ```sudo apt-get install libgtk-3-dev ```
2. use the following commands to compile the client C code with GTK3 (server.c is compiled normally):  
    ```
    gcc -o client client.c $(pkg-config --cflags --libs gtk+-3.0)
    
3. run the server first
4. you can then run up to 10 different clients
5. upon running a client, first enter a username of up to 10 characters
6. you can start sending messages of length up to 280 characters
