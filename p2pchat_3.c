#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"
#include "ui.h"

#define MAX_NUM_CONNECTIONS 10
#define MAX_USERNAME_LENGTH 16
#define MAX_MESSAGE_LENGTH 1024
#define MAX_CONDENSED_STRING_LENGTH MAX_USERNAME_LENGTH + MAX_MESSAGE_LENGTH + 2
#define MAGIC_NUMBER 10551055 //The list of peers is initialized to become an array of magic numbers

// Keep the username in a global so we can access it from the callback
const char *username;
// Number of connections done
int num_connections = 0;

// A single peer's lists of connections it has
int connected_peer_sockets[MAX_NUM_CONNECTIONS];

// Message struct that holds username and message
typedef struct message {
    char *username;
    char *message;
} message_t;

// De-scrample the message and convert it into a string, and return it
// char *turn_struct_to_string(message_t msg) {
//     message_t msg; 

//     char *condensed_string = malloc(sizeof(char) * (MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH + 2));

//     memcpy(condensed_string, msg.username, MAX_USERNAME_LENGTH + 1);

//     ui_display("memcpy user", condensed_string); 

//     memcpy(condensed_string + MAX_USERNAME_LENGTH + 1, msg.message, MAX_MESSAGE_LENGTH + 1);

//     ui_display("memcpy message", condensed_string); 
    
//     return condensed_string;
// }

// Convert a whole string with username+message to message struct, and return it
// message_t turn_string_to_struct(char *condensed_string) {
//     message_t msg;

//     msg.message = malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
//     msg.username = malloc(sizeof(char) * MAX_USERNAME_LENGTH);

//     memcpy(msg.username, condensed_string, MAX_USERNAME_LENGTH + 1);
//     memcpy(msg.message, condensed_string + MAX_USERNAME_LENGTH + 1, MAX_MESSAGE_LENGTH + 1);

//     return msg;
// }

// Runs the "server" for each peer. It runs in an infinite loop in a single thread of its own to accept new incoming connections
void *server_func(void *server_socket_fd_arg) {
    int port_server_fd = *(int *)server_socket_fd_arg;  // Converting thread arg to usable int

    // Start listening for connections, with a maximum of one queued connection
    // CITATION: Netwoeking exercise
    if (listen(port_server_fd, 1)) {
        perror("WError while listenign");
        exit(EXIT_FAILURE);
    }

    // Infinite loop
    while (true) {
        // CITATION: Netwoeking exercise
        int peer_socket_fd = server_socket_accept(port_server_fd);
        if (peer_socket_fd == -1) {
            perror("Error opening port");
            exit(EXIT_FAILURE);
        }
        if (num_connections > MAX_NUM_CONNECTIONS) {
            perror("Cannot exceed 20 connections");
            exit(EXIT_FAILURE);
        }
        ui_display("New port","Connected");
        // Adding the peer socket fd to the list of peers that are connected to us
        connected_peer_sockets[num_connections++] = peer_socket_fd;
    }
}

// Runs the "client" function of a peer. Repeatedly listens for new messages and displays it
void *listener_func() {
    // infinite loop
    while (true) {
        // Run through all the connections we have to periodically recieve messages
        for (int i = 0; i < num_connections; i++) {
            char *username = malloc(sizeof(char) * MAX_CONDENSED_STRING_LENGTH);  // Condensed string holds the "entire" message: username + message.
            char *message = malloc(sizeof(char) * MAX_CONDENSED_STRING_LENGTH); 
            // sleep(3);  // The UI cant seem to handle so many loops at once, it bugs out, and gives incpnsistennt behaviour. Hence, we sleep for a short while here
            // occasionally, messages take a while to appear due to this.

            if (connected_peer_sockets[i] == -1) break;
            // Read from peer socket
            username = receive_message(connected_peer_sockets[i]); 
            message = receive_message(connected_peer_sockets[i]); // CITATION: Networking Exercise, send_message and recieve_message functions
            // message_t msg = turn_string_to_struct(condensed_string);
            if ((username != NULL)||(message != NULL)) {  // There is something to display
                ui_display(username, message);

                for (int j = 0; j < num_connections; j++) {
                    if (connected_peer_sockets[j] == MAGIC_NUMBER) break;
                    if (i != j) {  // Avoid sending message to oneself (the same peer that it is originating from)
                        if (send_message(connected_peer_sockets[j], username) == -1) {  // CITATION: Netwoeking exercise
                            perror("Could not send message to peer");
                            exit(EXIT_FAILURE);
                        } else {
                            continue;
                        }

                        if (send_message(connected_peer_sockets[j], message) == -1) {  // CITATION: Netwoeking exercise
                            perror("Could not send message to peer");
                            exit(EXIT_FAILURE);
                        } else {
                            continue;
                        }
                    }
                }

            } else {  // There is nothing to display
                free(username);
                free(message);
                // free(condensed_string);
                continue;
            }
            free(username);
            free(message);
            // free(condensed_string);
        }
    }
}

// This function is run whenever the user hits enter after typing a message
void input_callback(const char *message) {
    if (strcmp(message, ":quit") == 0 || strcmp(message, ":q") == 0) {
        ui_exit();
    } else {
        ui_display(username, message);

        // Loop through and send to all available peers
        for (int j = 0; j < num_connections; j++) {
            // message_t msg;  // define a message struct

            // msg.message = malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
            // msg.username = malloc(sizeof(char) * MAX_USERNAME_LENGTH);
            // memcpy(msg.username, username, MAX_USERNAME_LENGTH);
            // memcpy(msg.message, message, MAX_MESSAGE_LENGTH);

            // ui_display("struct user", msg.username); 
            // ui_display("struct message", msg.message); 

            // ui_display("converting to string", turn_struct_to_string(msg)); 

            // convert the struct to string so that we can send to the connected peers
            // char *condensed_message = turn_struct_to_string(msg);  // condensed message is the "entire" string: username+message

            // ui_display("msg", condensed_message); 

            if (connected_peer_sockets[j] == MAGIC_NUMBER) break;  // Dont send to disconnected peers or peers not connetcted yet
            // if (j != MAGIC_NUMBER) {
                if (send_message(connected_peer_sockets[j], username) == -1) {  // CITATION: Networking Exercise, send_message and recieve_message functions
                    perror("Could not send message to peers");
                    exit(2);
                } else {
                    continue;
                }

                if (send_message(connected_peer_sockets[j], message) == -1) {  // CITATION: Networking Exercise, send_message and recieve_message functions
                    perror("Could not send message to peers");
                    exit(2);
                } else {
                    continue;
                }
            // }
            free(msg.username);
            free(msg.message);
            free(condensed_message);
        }
    }
}


// this function initializes each element in the peer list to the magic number. It is updated to the socket_fd of the peer as more peers are added
void init_peer_lst(int peer_lst[]) {
    for (int i = 0; i < MAX_NUM_CONNECTIONS; i++) {
        peer_lst[i] = MAGIC_NUMBER;
    }
}

int main(int argc, char **argv) {
    // Make sure the arguments include a username
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Usage: %s <username> [<peer> <port number>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Save the username in a global
    username = argv[1];

    // We initialize the peer list with a magic number. The list stays the magic number until a peer is connected, at which point, it is changed.
    // We check magic number to see if a peer has quit or not
    // CITATION: The idea of initializing the list to a list of MAGIC numbers, so that we can keep track of which peer has exited and which peer has not. was provided by Chat GPT
    init_peer_lst(connected_peer_sockets);

    // Set up a server socket to accept incoming connections
    unsigned short server_port = 0;
    int port_socket_fd = server_socket_open(&server_port);
    if (port_socket_fd == -1) {
        perror("Error in opening server socket");
        exit(EXIT_FAILURE);
    }

    // Did the user specify a peer we should connect to?
    if (argc == 4) {
        // Unpack arguments
        char *peer_hostname = argv[2];
        unsigned short peer_port = atoi(argv[3]);
        // connect to peer port
        int peer_socket_fd = socket_connect(peer_hostname, peer_port);
        if (peer_socket_fd == -1) {
            perror("Failed to connect to peer");
            exit(EXIT_FAILURE);
        }
        connected_peer_sockets[num_connections++] = peer_socket_fd;
    }

    // run listen thread
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_func, &port_socket_fd);
    pthread_t listener_thread;
    pthread_create(&listener_thread, NULL, listener_func, NULL);

    // Set up the user interface. The input_callback function will be called
    // each time the user hits enter to send a message.
    ui_init(input_callback);

    // Once the UI is running, you can use it to display log messages
    char port_string[50];
    sprintf(port_string, "%u", server_port);
    ui_display("LISTENING ON PORT", port_string);

    // Run the UI loop. This function only returns once we call ui_stop() somewhere in the program.
    ui_run();

    return 0;
}
