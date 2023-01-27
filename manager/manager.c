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
#include <signal.h>
#include <stdint.h>


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
int main(int argc, char **argv) {
    int pipe_file_descriptor;
    int mbroker_pipe;
    char register_pipe_name[PIPE_NAME_LEN];
    char pipe_name[PIPE_NAME_LEN];
    char command[OPTION_LEN];
    char box_name[BOX_NAME_LEN];

    if ((argc < 4) | (argc > 5)) {
        fprintf(stderr, "Usage: %s <register_pipe_name> <pipe_name> <command> <box_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    strcpy(register_pipe_name,argv[1]);
    strcpy(pipe_name,argv[2]);
    strcpy(command,argv[3]);

    if(argc == 5){
            strcpy(box_name,argv[4]);

    }
   else{ 
    snprintf(box_name,sizeof(box_name),"%s","");
   }


   // Create the named pipe
    if (mkfifo(pipe_name, 0666) < 0) {
        perror("Error creating Client named pipe");
        exit(EXIT_FAILURE);
    }
    // Open the named pipe
    pipe_file_descriptor = open(pipe_name,O_RDONLY | O_NONBLOCK);
    if (pipe_file_descriptor < 0) {
    // an error occurred opening the pipe
        perror("Error opening Client named pipe");
        exit(EXIT_FAILURE);
    }
    //open the mbroker pipe

    mbroker_pipe = open(register_pipe_name,O_WRONLY );
    if( mbroker_pipe < 0){
        perror("Error opening MBROKER named pipe");
        exit(EXIT_FAILURE);
    }
    //send message to mbroker
    uint8_t code = 0;

     if(!strcmp(command,"create")){
        code = 3;
    }
    if(!strcmp(command,"remove")){
        code = 5;
    }
    if(!strcmp(command,"list")){
        code = 7;
    }
  struct message arg_message = {
            .code = code,
            .pipe_name = {0},
            .box_name = {0},
            .message = {0}
    };

    if(code == 3 || code == 5){
        memcpy(arg_message.pipe_name,pipe_name, sizeof(pipe_name));
        memcpy(arg_message.box_name,box_name,sizeof(box_name));


    }
    else if(code == 7){
        memcpy(arg_message.pipe_name,pipe_name, sizeof(pipe_name));

    }
    

    //write to mbroker pipe the message
    if (write(mbroker_pipe, &arg_message, sizeof(arg_message)) < 0) {
        perror("Error writing to mbroker named pipe");
        exit(EXIT_FAILURE);
    }
    if(close(mbroker_pipe) < -1){
        printf("%s\n","error with closing mbroker pipe");
        exit(EXIT_FAILURE);
    }




    //read from the pipe
    ssize_t bytes_read;
    struct response res = {
                    .code = 4,
                    .return_code = 0,
                    .error = {0},
    };
    

    while(true){

        //waits for the mbroker
         bytes_read = read(pipe_file_descriptor, &res, sizeof(res));

        if(bytes_read > 0){
            break;
        }

    }

   
    code = res.code;
     int32_t return_code;
     return_code = res.return_code;


    
    if(((code == 4) | (code ==6)) && return_code == 0){
        fprintf(stdout, "OK\n");
    }
    else if(((code == 4) | (code ==6)) && return_code == -1){
        char error_message[MAX_MESSAGE_SIZE-10];
        memcpy(error_message, res.error, sizeof(error_message));
        fprintf(stdout, "ERROR %s\n", error_message);
    }
    
    
    if (close(pipe_file_descriptor) < 0) {
    // an error occurred while closing the pipe
        perror("Error closing named pipe");
        exit(EXIT_FAILURE);
    }

    return 0;
}
