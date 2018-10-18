#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#define CMDSIZE 100
#define ARGSIZE 10

/*Header file for the shell command:
    This file includes functions that process the internal
    commands of the shell, as well as other various functions.
*/

extern int erno;

int redirect_io(char **cmd_argv);
void parse_cmd(char *input, char *cmd_argv[]);

/* Change directory command: $cd
    This internal command changes the current working directory to the
    directory specified as the second arguement of the command line input.
    If no arguements follow the cd command, the directory shall change to
    the HOME directory. *MUST use "./" to specify relative path.*
*/
int cd_cmd(char **cmd_argv){

    //builds string of new path
    char *newDir = malloc(strlen(getenv("PWD")) + 50);
    strcpy(newDir, getenv("PWD"));
    //printf("newDir: %s\n", newDir);
    if(cmd_argv[1] == NULL){ //set to home
        strcpy(newDir, getenv("HOME"));

    }else if(strncmp(cmd_argv[1], "./", 2) == 0){ //relative path
        cmd_argv[1]++;
        strcat(newDir, cmd_argv[1]);
    }else{ //absolute path
        if(*cmd_argv[1] != '/'){ //adds "/" at beginning if input did not include
            strcpy(newDir, "/");
            strcat(newDir, cmd_argv[1]);
        }else{
            strcpy(newDir, cmd_argv[1]);
        }
        
    }

    //printf("newDir2: %s\n", newDir);

    //changes PWD to givin path if it exists
    struct stat sb;
    if(stat(newDir, &sb) == 0 && S_ISDIR(sb.st_mode)){
        //puts("exists");
        setenv("PWD", newDir, 1);
        if(chdir(newDir) == 0){
            ;//printf("success: %s\n", getenv("PWD"));
        }else{
            perror("Unable to change directory!");
        }
    }else{
        perror("Unable to change directory!");
    }

    free(newDir);

    return 0;
}

/* clear command: $clr
    This internal command clears the screen of the shell.
*/
int clr_cmd(){
    printf("\033[H\033[2J");
    return 0;
}

/* directory command: $dir
    Lists contents of the directory provided by the second arguement.
    If there are no arguements passed with the command, the contents of
    the current working directory are printed.
*/
int dir_cmd(char **cmd_argv){

    //get output column length to prevent word wrapping in between a string
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    unsigned MAX_col_len = w.ws_col;
    unsigned cur_col_len = 0;
    unsigned num_cols = 1;

    //creates directory and directory path string
    DIR *dir;
    struct dirent *S;
    char *directory = malloc(sizeof(strlen(getenv("PWD"))) + 50);
    int in = 1; // input directory arguement index
    //int out = 1; // output directory arguement index
    int i = 1;
    while(cmd_argv[i] != NULL){
        if((strcmp(cmd_argv[i], ">") == 0) || (strcmp(cmd_argv[i], ">>") == 0)){
            in = i - 1;
        }else if(strcmp(cmd_argv[i], "<") == 0){
            in = i + 1;
        }
        i++;
    }

    if(cmd_argv[1] == NULL || in == 0){ //if no path arguement
        strcpy(directory, getenv("PWD"));
    }else{ //set directory to path arguement
        if(*cmd_argv[in] != '/'){ //adds "/" at beginning if input did not include
            strcpy(directory, "/");
            strcat(directory, cmd_argv[in]);
        }else{
            strcpy(directory, cmd_argv[in]);
        }
    }

    char *dir_str = malloc(sizeof(char) * MAX_col_len);
    //outputs directory contents
    dir = opendir(directory);
    if(dir){
        while((S = readdir(dir)) != NULL){
            //controls word wrapping on output
            cur_col_len += (strlen(S->d_name) + 5);
            if(cur_col_len >= MAX_col_len){
                num_cols++;
                dir_str = realloc(dir_str, sizeof(char) * (MAX_col_len * num_cols));
                strcat(dir_str, "\n     ");
                strcat(dir_str, S->d_name);
                cur_col_len = 0;
            }else{
                strcat(dir_str, "     ");
                strcat(dir_str, S->d_name);
            }   
        }
        strcat(dir_str,"\n");
    }else{
        perror("Unable to open directory!");
    }

    closedir(dir);

    redirect_io(cmd_argv);
    printf("%s\n", dir_str);
    free(dir_str);

    return 0;
}

/* environ command: $environ
    This internal command lists all current environment variables
*/
int environ_cmd(char **cmd_argv, char *environ[]){
    redirect_io(cmd_argv);
    //iterates and prints every variable in the environment array
    for(int i = 0; environ[i] != NULL; i++){
        printf("%s\n", environ[i]);
    }

    return 0;
}

/* echo command: $echo
    This file will display the command line input following the echo command
*/
int echo_cmd(char **cmd_argv){
    redirect_io(cmd_argv);

    int i = 1;
    int j = 1;

    //if input redirection is used, echo reads from input file arguement
    while(cmd_argv[j] != NULL){
        if((strcmp(cmd_argv[j], "<") == 0)){
            i = j + 1;
            break;
        }
        j++;
    }
    if(i > 1){ //input file found
        char c;
        FILE *in = fopen(cmd_argv[i], "r");
        if(in == NULL){
            perror("Cannot open input file\n");
            return 0;
        }
        c = fgetc(in);
        while(c != EOF){ //read from input file
            printf("%c", c);
            c = fgetc(in);
        }
        printf("\n");
        fclose(in);  
        return 0;      
    }

    //read from command line
    while(cmd_argv[i] != NULL){
        //only print up to output file redirect character
        if((strcmp(cmd_argv[i], ">") == 0) || (strcmp(cmd_argv[i], ">>") == 0)){
            break;
        }
       printf("%s ", cmd_argv[i]);
       i++;
    }
    printf("\n");
    return 0;
}

/* Help command: $help
    Prints out the user manual for the shell.
*/
int help_cmd(char **cmd_argv){

    redirect_io(cmd_argv); //check redirection

    if(cmd_argv[1] == NULL){ //prints full readme manual
        char c;
        FILE *in = fopen("readme", "r");
        if(in == NULL){
            perror("Cannot open manual\n");
            return 0;
        }
        c = fgetc(in);
        while(c != EOF){ //read from "readme" file
            printf("%c", c);
            c = fgetc(in);
        }
        printf("\n");
        fclose(in); 

    }else{ //prints specific command manuals
        if(strcmp(cmd_argv[1], "cd") == 0){ 
            printf("\nChange directory command: $cd\n"
                "\tThis internal command changes the current working directory to the\n"
                "\tdirectory specified as the second arguement of the command line input.\n"
                "\tIf no arguements follow the cd command, the directory shall change to\n"
                "\tthe HOME directory. *MUST use \"./\" to specify relative path.*\n\n");
        }
        else if(strcmp(cmd_argv[1], "clr") == 0){
            printf("\nClear screen command: $clr\n"
            "\tThis internal command clears the screen of the shell.\n\n");
        }
        else if(strcmp(cmd_argv[1], "dir") == 0){
            printf("\nPrint directory command: $dir\n"
                "\tLists contents of the directory provided by the second arguement.\n"
               "\tIf there are no arguements passed with the command, the contents of\n"
               "\tthe current working directory are printed.\n"
               "\tNOTE: The full path name of the arguement must be provided!\n"
                    "\t\tgood: $:~/SeansShellPath> dir /home/TU/tuh42070/CIS3207/shell\n"
                    "\t\tbad:  $:~/SeansShellPath> dir /shell\n\n");
        }
        else if(strcmp(cmd_argv[1], "environ") == 0){
            printf("\nEnviron command: $environ\n"
                "\tThis internal command lists all current environment variables.\n\n");

        }
        else if(strcmp(cmd_argv[1], "echo") == 0){
            printf("\nEcho command: $echo\n"
                "\tThis file will display the command line input following the echo command.\n\n");
        }
        else if(strcmp(cmd_argv[1], "help") == 0){
            printf("\nHelp command: $help\n"
                "\tPrints out the user manual for the shell. Command specific commands can also\n"
                "\tbe displayed by entering the desired command as the first arguement:\n"
                    "\t\t$:~/SeansShellPath>help dir\n\n");
        }
        else if(strcmp(cmd_argv[1], "pause") == 0){
            printf("\nPause command: $pause\n"
                "\tPause operation of the shell until 'Enter' is pressed.\n\n");
        }
        else if(strcmp(cmd_argv[1], "quit") == 0){
            printf("\nQuit command: $quit\n"
                "\tExits out of Sean's Shell and ends its process.\n\n");
        }
        else{
            printf("%s\n", "The specified manual does not exist!");
        } 
    }
 

    return 0;
}

/* Pause command: $pause
    pause operation of the shell until 'Enter' is pressed.
*/
int pause_cmd(){
    printf("Shell paused - press 'ENTER' to continue...");
    char c = getchar();
    //waits for input of 'Enter' key
    while(c != '\n'){
        c = getchar();
    }

    return 0;
}

/* Quit command: $quit
    Handles if the user enters the quit command to exit the shell
*/
int quit_cmd(char *cmd){
    if(strcmp(cmd, "quit") == 0){
        return 0;
    }else if(strcmp(cmd, "QUIT") == 0){
        return 0;
    }else{
        return 1;
    }
}

/* Determines if the entered command is an internal command.
    If an internal command has been entered, the corrisponding function is called.
    Each function returns 0 if successful or 1 if there was an error
*/
int internal_command(char **argv, char *environ[]){

    int error;

    if(strcmp(argv[0], "cd") == 0){ //change directory
        error = cd_cmd(argv);
    }
    else if(strcmp(argv[0], "clr") == 0){ //clear screen
        error = clr_cmd();
    }
    else if(strcmp(argv[0], "dir") == 0){ //list directory
        error = dir_cmd(argv);
    }
    else if(strcmp(argv[0], "environ") == 0){ //print environment
        error = environ_cmd(argv, environ);
    }
    else if(strcmp(argv[0], "echo") == 0){ //print comment
        error = echo_cmd(argv);
    }
    else if(strcmp(argv[0], "help") == 0){ //print manual
        error = help_cmd(argv);
    }
    else if(strcmp(argv[0], "pause") == 0){ //pause shell
        error = pause_cmd();
    }
    else{
        error = -1;
    }

    return error;
}

/* Determines if the entered command is an external command.
    This function searches for the existence of the command
    through the directories of try_dir[]. If found, the directory
    of the external command is returned; returning null if the 
    command does not found.
*/
char *external_command(char **argv){

    char *try_dir[] = {"./", "/bin/", "/usr/bin/", NULL};
    char *ex_path = malloc(sizeof(strlen(getenv("PWD"))) + 50);
    int i = 0;   
    while(try_dir[i] != NULL){
        strcpy(ex_path, try_dir[i]);
        strcat(ex_path, argv[0]);
        //printf("ex_path: %s\n", ex_path); 
        int ex_cmd = open(ex_path, O_RDONLY);
        if(ex_cmd > 0){
            //printf("exists at %d:\n", i);
            close(ex_cmd);
            return ex_path;
        }else{
            //printf("ex err %d: %s\n", i, strerror(errno));
            close(ex_cmd);
        }
        i++;
    }

    //printf("%s\n", strerror(errno));
    free(ex_path);
    return NULL;
}

/* A process can be executed in the background of the shell. To do so,
    an '&' character must be at the end of a command line. This function
    determines if the passed command is to be executed in the background
    of the shell.
    
*/
int is_background_cmd(char **cmd_argv){

    int i = 1;
    while(cmd_argv[i] != NULL){
        if(strcmp(cmd_argv[i], "&") == 0 && cmd_argv[i+1] == NULL){
            return 1;
        }
        i++;
    }

    return 0;
}

/* A command's input or output file descriptors can be specified in
    the command line. This allows the user to change where the entered command
    process (cmd_argv[0]) reads and writes from. i/o redirection can be done
    using the following characters:

    '<'  - Redirects the input for the cmd process from the preceding arguement.

    '>'  - Redirects the output of the cmd process to the proceeding arguement.
            If the output file exists, the output will TRUNCATE the existing file.
            If the output file does not exist, a new file will be created.

    '>>' - Redirects the output of the cmd process to the proceeding arguement.
            If the outfile exists, the output will be APPENDED to the file.
            If the output file does not exist, a new file will be created.
    
    This function will determine if the command line contains redirection characters,
     and will change the file descriptors accordingly.
*/
int redirect_io(char **cmd_argv){

    int num_redirects = 0; //debugging purposes
    int i = 1;
    while(cmd_argv[i] != NULL){

        // <
        if(strcmp(cmd_argv[i], "<") == 0){ //redirect input
            int newstdin = open(cmd_argv[i+1],O_RDONLY);
            if(newstdin > 0){ //successful redirection
                close(0);
                dup2(newstdin, 0);
                close(newstdin);
				cmd_argv[i] = NULL;
                num_redirects++;
                i++;
            }else{
                close(newstdin);
                perror("redirect input failed: ");
            }
        }

        // >
        if(strcmp(cmd_argv[i], ">") == 0){ //redirect output
			
            int newstdout = open(cmd_argv[i+1],O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
            if(newstdout > 0){ //successful redirection
                close(1);
                dup2(newstdout, 1);
                close(newstdout);
				cmd_argv[i] = NULL;
                num_redirects++;
                break;
            }else{
                close(newstdout);
                perror("redirect output failed: ");
            }
        }

        // >>
        if(strcmp(cmd_argv[i], ">>") == 0){ //redirect output
            int newstdout = open(cmd_argv[i+1],O_WRONLY|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO);
            if(newstdout > 0){ //successful redirection
                close(1);
                dup2(newstdout, 1);
                close(newstdout);
				cmd_argv[i] = NULL;
                num_redirects++;
                break;
            }else{
                close(newstdout);
                perror("redirect output failed: ");
            }
        }

        i++;
    }

    return num_redirects;
}

/*Returns a new cmd_argv without shell options (i.e. <, >, >>, |, &)
    for use with external commands
*/
char **clean_argv(char **cmd_argv){

    char **new_argv = malloc(sizeof(char *) * ARGSIZE);
    new_argv[0] = malloc(sizeof(char) * 20);
    int i = 0; //argv index
    int j = 0; //new_argv index

    //add each nonshell option from argv arguement to new_argv
    while(cmd_argv[i] != NULL){
        
        if((strcmp(cmd_argv[i], "<") != 0)
        && (strcmp(cmd_argv[i], ">") != 0)
        && (strcmp(cmd_argv[i], ">>") != 0)
        // && (strcmp(cmd_argv[i], "|") != 0)
        && (strcmp(cmd_argv[i], "&") != 0)){
            new_argv[j] = malloc(sizeof(char) * 20);
            strcat(new_argv[j], cmd_argv[i]);
            j++;
        }
        i++;
    }
    new_argv[j] = NULL;

    return new_argv;
}

//Checks if there is a pipe operator in the command line.
int pipe_check(char **cmd_argv){

    int i = 1;
    int is_pipe = 0;
    while(cmd_argv[i] != NULL){
        if(strcmp(cmd_argv[i], "|") == 0){
            is_pipe = i;
            return is_pipe;
        }
        i++;
    }    
    return is_pipe;
}

//Changes which part of the command line each side of the pipe needs to be.
void pipe_set(char **cmd_argv, int pid){
    
    int i = 1;
    while(cmd_argv[i] != NULL){
        if(strcmp(cmd_argv[i], "|") == 0){
            if(pid > 0){ //if parent, change cmd_argv to only the right half "|" operator
                int j = 0;
                while(cmd_argv[j] != NULL){ //shift array elements loop
                    if(cmd_argv[i] != NULL){
                        cmd_argv[i++] = NULL;
                        strcpy(cmd_argv[j], cmd_argv[i]);
                    }
                    j++;
                }
            }else if(pid == 0){
                while(cmd_argv[i] != NULL){
                    cmd_argv[i] = NULL; //if child, change cmd_argv to only the left half of "|" operator
                    i++;
                }
                
            }
        }
        i++;
    }
}

//processes the command line, cmd_argv, as either an internal or external command.
int process_command_line(char **cmd_argv, char **environ){

     if(cmd_argv[0] != NULL){ //if the input arg is valid

        

        if(quit_cmd(cmd_argv[0]) == 0){ //quit shell check
            //done = 1;
            return 1;
        }
        
        int cmd_type = internal_command(cmd_argv, environ);
        if(cmd_type >= 0){ //internal command
            if(cmd_type > 0){
                perror("ERROR EXECUTING INTERNAL COMMAND!");
            }
        }else{
            int run_in_background = is_background_cmd(cmd_argv); //checks &              
            if(redirect_io(cmd_argv) || run_in_background){
                cmd_argv = clean_argv(cmd_argv);
            };
            char *cmd_path = external_command(cmd_argv);
            if(cmd_path != NULL){
                int pid = fork();
                if(pid >= 0){ //no error
                    if(pid == 0){ //child 1

                        //pipe
                        int is_pipe = pipe_check(cmd_argv);
                        int pid2;
                        int fd[2];                        

                        if(is_pipe > 0){
                            if(pipe(fd) == 0){
                                pid2 = fork();
                                if(pid2 == 0){ //child 2
                                        close(1); //close stdout
                                        dup2(fd[1], 1); //put stdout into pipe's write end
                                        close(fd[0]); //close pipe's read end

                                        pipe_set(cmd_argv, pid2);
                                        cmd_path = external_command(cmd_argv);
                                        if(execvp(cmd_path, cmd_argv) < 0){
                                            perror("ERROR EXECUTING PARENT'S PIPE COMMAND!");
                                            exit(0);
                                        }  
                                }else if(pid2 > 0){ //parent 2
                                    int status2 = 0;
                                    //wait(&status2);
                                    close(0); //close stdin
                                    dup2(fd[0], 0); //put stdin into pipe's read end
                                    close(fd[1]); //close pipe's write end
                                    pipe_set(cmd_argv, pid2);
                                    cmd_path = external_command(cmd_argv);
                                    if(execvp(cmd_path, cmd_argv) < 0){
                                        perror("ERROR EXECUTING CHILD'S PIPE COMMAND!");
                                        exit(0);
                                    }                    
                                }

                            }else{
                                perror("pipe failed!");
                            }
                        } //end pipe

                        else if(execvp(cmd_path, cmd_argv) < 0){
                            perror("ERROR EXECUTING EXTERNAL COMMAND!");
                            exit(0);
                        }
                    } else{ //parent 1
                        int status = 0;
                        if(!run_in_background){
                            wait(&status);
                            //printf("Child exited with status of %d\n", status);   
                        }else{
                            //printf("%s\n", "Child process executing in background.."); 
                        }            
                    }
                } else{
                    perror("FORK ERROR!");
                }
            }else{
                perror(cmd_argv[0]);
                perror(": invalid command.\n");
            }  
        }
    }

    return 0;
}

//Parses the passed input input into an array of arguements
void parse_cmd(char *input, char *cmd_argv[]){
    char *tokenPtr = strtok(input, "\n\t "); //get first token (input)

    cmd_argv[0] = tokenPtr;
    int index = 1;

    //parse remaining args until NULL
    while(tokenPtr != NULL){
        tokenPtr = strtok(NULL, "\n\t ");
        cmd_argv[index] = tokenPtr;
        if(tokenPtr != NULL){
            index++;
        }
    }
    free(tokenPtr);
    //set remaining empty indexes to null
    while(index < ARGSIZE){
        cmd_argv[index] = NULL;
        index++;
        
    }
}