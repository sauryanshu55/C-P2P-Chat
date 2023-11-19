#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "socket.h"
#include "ui.h"

#define MAX_USERS 10
#define MAX_MESSAGE_LENGTH 2048

// Keep the username in a global so we can access it from the callback
const char* username;
int peer_sockets[MAX_USERS];
int num_peers_connected=0;

void *server_func(void* server_socket_fd_arg){
  int server_socket_fd = *((int *)server_socket_fd_arg);
  if (listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  while(true){
    if (num_peers_connected>MAX_USERS) break;
    int client_socket_fd = server_socket_accept(server_socket_fd);
    if (client_socket_fd == -1) {
      perror("accept failed");
      exit(EXIT_FAILURE);
    }
    ui_display("Connected to port","PORT");
    num_peers_connected++;
    peer_sockets[num_peers_connected]=client_socket_fd;
  }
  return NULL;
}

void* listener_func(){
  char buf[500];
  while(true){
    for (int i=0;i<num_peers_connected;i++){
      int rd = read(peer_sockets[i], buf, 500);
      if(rd == -1){
        perror("Could not receive message");
        exit(EXIT_FAILURE);
        }
      if (buf != NULL) {
        ui_display("CLIENT", buf);
      }
      continue;
    }
  }
  return NULL;
}

// This function is run whenever the user hits enter after typing a message
void input_callback(const char* message) {
  if (strcmp(message, ":quit") == 0 || strcmp(message, ":q") == 0) {
    ui_exit();
  } else {
    ui_display(username, message);
  
    for (int i; i<num_peers_connected;i++){
      if(write(peer_sockets[i], message, 500) == -1){
        perror("Could not write to peer");
        exit(2);
      }
    }
  }
}

int main(int argc, char** argv) {
  // Make sure the arguments include a username
  if (argc != 2 && argc != 4) {
    fprintf(stderr, "Usage: %s <username> [<peer> <port number>]\n", argv[0]);
    exit(1);
  }

  // Save the username in a global
  username = argv[1];

  // TODO: Set up a server socket to accept incoming connections
  // Open a server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(EXIT_FAILURE);
  }

  // Did the user specify a peer we should connect to?
  if (argc == 4) {
    // Unpack arguments
    char* peer_hostname = argv[2];
    unsigned short peer_port = atoi(argv[3]);

    // TODO: Connect to another peer in the chat network
    int socket_fd = socket_connect(peer_hostname, peer_port);
    // listen here!
    if (socket_fd == -1) {
      perror("Failed to connect");
      exit(EXIT_FAILURE);
    }

    num_peers_connected++;
    peer_sockets[num_peers_connected]=socket_fd;
  }

  pthread_t server_thread;
  pthread_create(&server_thread,NULL,server_func,&server_socket_fd);
  // dont make thread here idiot
  pthread_t listener_thread;
  pthread_create(&listener_thread,NULL,listener_func,NULL);


  // Set up the user interface. The input_callback function will be called
  // each time the user hits enter to send a message.
  ui_init(input_callback);

  // Once the UI is running, you can use it to display log messages
  ui_display("INFO", "This is a handy log message.");

  char port_string[50];
  sprintf(port_string,"%u",port);
  ui_display("Listening on port",port_string);

  // Run the UI loop. This function only returns once we call ui_stop() somewhere in the program.
  ui_run();

  return 0;
}
