#include "shell.h"
#define CMDSIZE 100
#define ARGSIZE 10

/*
    CIS3207-002: Lab 2 - SHELL
    October 6th, 2018
    By Sean Reddington
    Project description: This is my second project for the Temple University course:
        CIS3207 - Introduction to Systems Programming and Operating Systems

        This progam will allow the user to run my version of a shell within a 
        linux shell itself. While running Sean's Shell, the user will have access
        to both the internal commands built into the Shell as well as external commands
        from the linux environment its running in.
*/

int done = 0;

int main(int argc, char *argv[]){

	
	//set environment variable array
	char **environ;
	environ = argv;
	environ += 2;

    clr_cmd(); //clears console
    printf("\t\t\t*****Welcome to Sean's Shell!*****\n");


    char *shell_home_path = malloc(strlen(getenv("PWD")) + 1);
    strcpy(shell_home_path, getenv("PWD"));

    //save std file descriptors
    int stdin_copy = dup(0);
    int stdout_copy = dup(1);


    //main loop which processes a single input input each iteration
    while(!done){
        
        //gets pwd from current environment
        char *cur_path = malloc(strlen(getenv("PWD")) + 50);
        strcpy(cur_path, getenv("PWD"));

        //read command line input from stdin into a string
        char cmd_input[CMDSIZE];

        printf("Seans-Shell:~%s>", cur_path); //print shell prompt
        fgets(cmd_input, CMDSIZE, stdin);

        
        //parses input and args from input string into string array
        char **cmd_argv = malloc(sizeof(char*) * ARGSIZE);
        parse_cmd(cmd_input, cmd_argv); 

            int process_results = process_command_line(cmd_argv, environ);
            if(process_results < 0){
                perror("command process failed!");
                break;
            }else if(process_results == 1){ //quit shell
                done = 1;
                break;
            }else{
                //puts("process success");
            }

        dup2(stdin_copy, 0);
        dup2(stdout_copy, 1);

        free(cur_path);
    } //end while loop

    free(shell_home_path);
    
    puts("EXITING SEAN'S SHELL");
	
    return 0;
}//end main
