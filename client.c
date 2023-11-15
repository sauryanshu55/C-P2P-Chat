#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "message.h"
#include "socket.h"
char *capitalize_all(char *str) {
  for (int i = 0; str[i]; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 32;
    }
  }
  return str;
}


int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <server name> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Read command line arguments
  char* server_name = argv[1];
  unsigned short port = atoi(argv[2]);

  // Connect to the server
  int socket_fd = socket_connect(server_name, port);
  if (socket_fd == -1) {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }

  bool quit=false;
  while(!quit){
    char *input = NULL;
    size_t size = 0;
    ssize_t characters_read;

    printf("Message: ");

    // Use getline to read input from stdin (standard input)
    characters_read = getline(&input, &size, stdin);
    input[strlen(input)-1]='\0';

    int rc = send_message(socket_fd,input);
    if (rc == -1) {
      perror("Failed to send message to server");
      exit(EXIT_FAILURE);
    }


    if (strcmp(input,"quit")==0){
      quit=true;
      break;
    }
    // Read a message from the server
    char* message = receive_message(socket_fd);

    if (message == NULL) {
      perror("Failed to read message from server");
      exit(EXIT_FAILURE);
    }

    if (!quit){
      // Print the message
      printf("Server: %s\n", capitalize_all(message));
    }
    
    // Free the message
    free(message);
  }


  // Close socket
  close(socket_fd);

  return 0;
}
