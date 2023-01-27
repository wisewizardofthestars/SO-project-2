#include "logging.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <state.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>


struct response{
    uint8_t code;
    int32_t return_code;
    char error[MAX_MESSAGE_SIZE];
};

struct message{
    uint8_t code;
    char pipe_name[PIPE_NAME_LEN];
    char box_name[BOX_NAME_LEN];
    char message[MAX_MESSAGE_SIZE];

} ;

struct writing{
    uint8_t code;
    char message[MAX_MESSAGE_SIZE];
};



int main(int argc, char **argv) {
    struct message session= {
        .code = 2,
        .pipe_name ={0},
        .box_name = {0},
        .message = {0}
    };
    int pipe_file_descriptor;
    char register_pipe_name[PIPE_NAME_LEN];
    
    if (argc < 4) {
        fprintf(stderr, "Usage: %s  <register_pipe_name> <pipe_name> <box_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    strcpy(register_pipe_name,argv[1]);
    strcpy(session.pipe_name,argv[2]);
    strcpy(session.box_name,argv[3]);

   // Create the named pipe


    if (mkfifo(session.pipe_name, 0666) < 0) {
        perror("Error creating named pipe");
        exit(EXIT_FAILURE);
    }
    //open the client pipe
    pipe_file_descriptor = open(session.pipe_name, O_RDONLY | O_NONBLOCK);
    if( pipe_file_descriptor < 0){
        perror("Error opening client named pipe");
        exit(EXIT_FAILURE);
    }
    
    // Open the named pipe; mbroker
    
    int mbroker_pipe = open(register_pipe_name,O_WRONLY);

    if (mbroker_pipe < 0) {
    // an error occurred opening the pipe
        perror("Error opening mbroker named pipe");
        exit(EXIT_FAILURE);
    }    
    //write to mbroker pipe the message
    if (write(mbroker_pipe, &session, sizeof(session)) < 0) {
        perror("Error writing to mbroker named pipe");
        exit(EXIT_FAILURE);
    }
    //close the mbroker pipe
    if (close(mbroker_pipe) < 0) {
    // an error occurred while closing the pipe
        perror("Error closing mbroker named pipe");
        exit(EXIT_FAILURE);
    }
    //read from the pipe: no, if the session is accepted we keep going
    //read stdin and write to the mbroker
   
    struct writing answer = {
        .code = 10,
        .message = {0}
    };

    char input[MAX_MESSAGE_SIZE];
    ssize_t bytes_read;
    
    while (true) {
        bytes_read = read(pipe_file_descriptor, input, MAX_MESSAGE_SIZE);
      
        memcpy(answer.message, input, sizeof(answer.message));

         fprintf(stdout, "%s\n", answer.message);
        if(bytes_read <= 0){
            break;
        }

    }
    if (close(pipe_file_descriptor) < 0) {
    // an error occurred while closing the pipe
        perror("Error closing named pipe");
        exit(EXIT_FAILURE);
    }

    return 0;
}

