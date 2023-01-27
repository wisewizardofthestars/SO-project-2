#include "logging.h"
#include <unistd.h>
#include <stdio.h>
#include "config.h"
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "state.h"
#include <stdlib.h>
#include "operations.h"
#include <stdint.h>
#include <signal.h>
// #include "producer-consumer.h"

struct message{
    uint8_t code;
    char pipe_name[PIPE_NAME_LEN];
    char box_name[BOX_NAME_LEN];
    char message[MAX_MESSAGE_SIZE];

} ;

struct response{
    uint8_t code;
    int32_t return_code;
    char error[MAX_MESSAGE_SIZE];
};

struct answer{
    uint8_t code;
    char message[MAX_MESSAGE_SIZE];
};

int main(int argc, char **argv) {
    int pipe_file_descriptor;
    char register_pipe_name[PIPE_NAME_LEN];
     if (argc < 3) {
        fprintf(stderr, "Usage: %s <register_pipe_name> <max_sessions>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    strcpy(register_pipe_name,argv[1]);

    int max_sessions = atoi(argv[2]);
    if (max_sessions < 1) {
        fprintf(stderr, "Error: max_sessions must be a positive integer\n");
        exit(EXIT_FAILURE);
    }

    // Create the named pipe

    if (mkfifo(register_pipe_name, 0666) < 0) {
        perror("Error creating mbroker named pipe");
       exit(EXIT_FAILURE);
    }

    // Open the named pipe FOR THE REGISTRATION
    pipe_file_descriptor = open(register_pipe_name,O_RDONLY | O_NONBLOCK);


    if (pipe_file_descriptor < 0) {
    // an error occurred opening the pipe
        perror("Error opening mbroker named pipe");
        exit(EXIT_FAILURE);
    }

    //i have to inicatilize the tf file system so i can store the inboxes there
    if(tfs_init(NULL) < 0){
        perror("Error initializing the tfs file system");
        exit(EXIT_FAILURE);
    }

 // Read messages from the pipe and process them
   
    // while loop to read the messages to the pipe
    struct message arg_message = {
            .code = 0,
            .pipe_name = {0},
            .box_name = {0},
            .message = ""
    };
    
    char box_name[BOX_NAME_LEN];
    char client_pipe[PIPE_NAME_LEN];
    ssize_t bytes_read;
    char message [MAX_MESSAGE_SIZE];

    while(true){
        // read the message from the pipe
        bytes_read = read(pipe_file_descriptor, &arg_message, sizeof(arg_message));

        //if theres no message keep waiting
        if(bytes_read > 0){


         uint8_t code = arg_message.code;
        memcpy(client_pipe,arg_message.pipe_name, sizeof(arg_message.pipe_name));
        memcpy(message,arg_message.message, sizeof(arg_message.message));

        memcpy(box_name,arg_message.box_name, sizeof(arg_message.box_name));
       


        char box_name_path[BOX_NAME_LEN  +1];
        char error_message[MAX_MESSAGE_SIZE -10] = "";
        int file_d ;
        printf("%d%s\n",code, "codigooo");
        switch (code) {

            case 1: 
                    struct answer inp = {
                        .code = 9,
                        .message = ""
                    };


                //open the box
                 file_d = open(client_pipe, O_RDONLY | O_NONBLOCK);
                 if(file_d < 0){
                 //opening the client pipe
                    printf("%s\n","ERRor openijng file to read");


                    exit(EXIT_FAILURE);
                 }     
                 snprintf(box_name_path, sizeof(box_name_path), "/%s", box_name);
                                    

                 int box_number_p = tfs_open(box_name_path, TFS_O_APPEND);
                  //couldnt open the box
                if(box_number_p < 0){
                    printf("%s\n","error opining box, doesnt exist");
                    exit(EXIT_FAILURE);
                }
                
                
                 while(true){
                    bytes_read = read(file_d, &inp, sizeof(inp));

                    if(bytes_read == 0){
                    break;
                   }
                   if(bytes_read > 0){

                      if(!tfs_write(box_number_p,inp.message, strlen(inp.message)+1)){
                        printf("%s\n","couldnt write in the box");
                        exit(EXIT_FAILURE);
                }
                   }
                   
                   
                 }
                  
            //close the box:
            

             if(tfs_close(box_number_p) < 0){
                printf("%s\n","couldnt close the box");
                    exit(EXIT_FAILURE);
                
            }
            if(!close(file_d)){
                printf("%s\n","couldnt close sub pipe");
                            exit(EXIT_FAILURE);
                
            }
                break;
            case 5: // mailbox deletion; client pipe

                
   
    
                 //construct the pathname for the box
                snprintf(box_name_path, sizeof(box_name_path), "/%s", box_name);

                 file_d = open(client_pipe, O_WRONLY);
                 if(file_d < 0){
                 //opening the client pipe
                    printf("%s\n","error opening client pipe");

                    exit(EXIT_FAILURE);
                 }           

                int box_handler_d = tfs_open(box_name_path, TFS_O_APPEND);
                struct response res_d = {
                    .code = 6,
                    .return_code = 0,
                    .error = {0}
                    };

                if(box_handler_d < 0){
  
                    snprintf(error_message,sizeof(error_message),"ERROR: box %s doesnt exist",box_name);
                    
                    res_d.return_code = -1;
                    memcpy(res_d.error,error_message, sizeof(error_message));
     
                    if(!write(file_d,&res_d, sizeof(res_d))){
                        close(file_d);
                        printf("%s\n","error writing to the client pipe manager delinting box");

                        exit(EXIT_FAILURE);
                    }
                    close(file_d);
                    printf("%s\n","error writing to the client pipe manager deleting box2");

                    exit(EXIT_FAILURE);
                 }
                //box exists
                 if(tfs_unlink(box_name_path) < 0) {
                    snprintf(error_message,sizeof(error_message),"ERROR: box %s couldnt be deleted",box_name);
                    
                    res_d.return_code = -1;
                    memcpy(res_d.error,error_message, sizeof(error_message));
                    if(!write(file_d,&res_d, sizeof(res_d))){
                        close(file_d);
                        printf("%s\n","error writing to the client pipe manager delinting box");

                        exit(EXIT_FAILURE);
                    }
                    close(file_d);
                    printf("%s\n","error writing to the client pipe manager deleting box2");

                    exit(EXIT_FAILURE);
                 }


                res_d.return_code = 0;
                memcpy(res_d.error,error_message, sizeof(error_message));


                if(!write(file_d,&res_d, sizeof(res_d))){
                    close(file_d);
                    printf("%s\n","error writing to the client pipe manager creating box3");

                    tfs_close(box_handler_d);
                    exit(EXIT_FAILURE);
                 }   
                close(file_d); 
                tfs_close(box_handler_d);
                break;
            case 3: // mailbox creation; client pipe
                
                //caso a caixa já exista não dá erro; é só um pedido redundante
    
                 //construct the pathname for the box
                snprintf(box_name_path, sizeof(box_name_path), "/%s", box_name);

                 file_d = open(client_pipe, O_WRONLY);
                 if(file_d < 0){
                 //opening the client pipe

                    exit(EXIT_FAILURE);
                 }           

                //create the box
                int box_handler = tfs_open(box_name_path, TFS_O_CREAT);
                struct response res = {
                    .code = 4,
                    .return_code = 0,
                    .error = {0}
                    };

                if(box_handler < 0){
                //box coulnt be created
                //error, return the error to the manager
                    snprintf(error_message,sizeof(error_message),"ERROR: box %s couldnt be created",box_name);
                    
                    res.return_code = -1;
                    memcpy(res.error,error_message, sizeof(error_message));

                    if(!write(file_d,&res, sizeof(res))){
                        close(file_d);
                        printf("%s\n","error writing to the client pipe manager creating box");

                        exit(EXIT_FAILURE);
                    }
                    close(file_d);
                    printf("%s\n","error writing to the client pipe manager creating box2");

                    exit(EXIT_FAILURE);
                 }
                //box created

                res.return_code = 0;
                memcpy(res.error,error_message, sizeof(error_message));
    
                if(!write(file_d,&res, sizeof(res))){
                    close(file_d);
                    printf("%s\n","error writing to the client pipe manager creating box3");

                    tfs_close(box_handler);
                    exit(EXIT_FAILURE);
                 }   
                close(file_d); 
                tfs_close(box_handler);

                break;
            case 2:
                //register the sub
  
                struct answer subsub={
                    .code = 10,
                    .message = {0}

                };
                
                snprintf(box_name_path, sizeof(box_name_path), "/%s", box_name);

                 file_d = open(client_pipe, O_WRONLY);
                 if(file_d < 0){
                 //opening the client pipe
                    printf("%s\n","error client pipe, sub");

                    exit(EXIT_FAILURE);

                 }           
                 int box_number_sub = tfs_open(box_name_path, TFS_O_APPEND);
                 //couldnt open the box
                if(box_number_sub < 0){
                    printf("%s\n","error opining box, doesnt exist");
                    exit(EXIT_FAILURE);
                }
                 //but i want to read only until the /0...
                 while(tfs_read(box_number_sub,subsub.message, sizeof(subsub.message))){
                    printf("%s\n",subsub.message);
                    if(!write(file_d,&subsub, sizeof(subsub))){
                             printf("%s\n","error writing to sub box");
                            exit(EXIT_FAILURE);
                    }

                 }
            //close the box:
            if(!tfs_close(box_number_sub)){
                printf("%s\n","couldnt close the box");
                            exit(EXIT_FAILURE);
                
            }
            if(close(file_d) == -1){
                printf("%s\n","couldnt close sub pipe");
                            exit(EXIT_FAILURE);
                
            }
            break;

            default:
                fprintf(stderr, "Error: invalid request type\n");
                break;
        }
    }
    }
    if(!tfs_destroy()){
        perror("Error destroying the tfs file system");
        exit(EXIT_FAILURE);
    }
    if(!close(pipe_file_descriptor)){
        perror("Error closing the mbroker named pipe");
        exit(EXIT_FAILURE);
    }
    
    return -1;
}