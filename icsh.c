/* ICCS227: Project 1: icsh
 * Name: Daran Thawornwattanapol
 * StudentID: 6380413
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "signal.h"
#include "fcntl.h"
#include "errno.h"
#include "sys/types.h"

#define MAX_CMD_BUFFER 255
pid_t pid;
pid_t foregroudPID; // foreground process id
int isInOrOut = -1;
int status_code;
int isBgjob;
int prevExitcode;

// Echo's helper function
void printString(char *args[])
{
    for (int i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);
        if (args[i + 1] != NULL)
        {
            printf(" ");
        }
    }
    printf("\n");
}

// Foreground job
void runForeground(char *args[])
{
    pid = fork();

    // child is sucessfully created
    if (pid == 0)
    {
        status_code = execvp(args[0], args);
        if (status_code == -1) {
            printf("bad command\n");
        }
        prevExitcode = 1;
        exit(1);
    }
    // child is unsuccessfully created
    else if (pid < 0)
    {
        perror("Child is unsuccessfully created");
        prevExitcode = 1;
        exit(1);
    }
    // Run in parent process
    else 
    {
        foregroudPID = pid;
        // Wait until the foreground process finished
        waitpid(pid, &status_code, 0);
        foregroudPID = 0;
    }
}

// SIGNAL HANDLER
void sig_handler(int signum) {
    /*Ctrl+Z*/
    if (signum == SIGTSTP && foregroudPID) {
        printf("PID: %d. Foreground process is stop(suspended).",foregroudPID);
        kill(foregroudPID, SIGTSTP);
        printf("\n");
    }
    /*Ctrl+C*/
    else if (signum == SIGINT && foregroudPID) {
        printf("PID: %d. Foreground process is killed.",foregroudPID);
        kill(foregroudPID, SIGINT);
        printf("\n");
    }
    else {
        printf("\n");
    }
}

// I/O Redirection
void reDirect(char *args[]) {
    /* if 0 , the output will be redirected to the spcific file = write
       If 1 , the input will be redirected from the spcific file = read
       3 Default file ids
       0 -> stdin
       1 -> stdout
       2 -> stderr
       */
    int file;
    char *fileName;
    char *command[4];
    int isEnd = 0;

    // get command and file name
    for (int i = 0; i < 4; i++)
    {
        if (args[i] != NULL) {
            if ((strcmp(args[i],">"))==0 || (strcmp(args[i],"<")==0))
            {
                isEnd = 1;
                command[i] = NULL;
                fileName = args[i+1];
            }
            else 
            {
                command[i] = args[i];
            }
        }
        else { // isEnd==1
            command[i] = NULL;
        }
    }
    
    // Input
    if (isInOrOut == 1) {
        file = open (fileName, O_RDONLY);
        if (file < 0) {
            fprintf (stderr, "Couldn't open a file\n");
            prevExitcode = errno;
            exit (errno);
        }
        }
    // Output
    else if (isInOrOut==0){
        file = open (fileName, O_TRUNC | O_CREAT | O_WRONLY, 0666);
        if (file < 0) {
            fprintf (stderr, "Couldn't open a file\n");
            prevExitcode = errno;
            exit (errno);
        }
    }
    //Run foreground
    pid = fork();

    // child is sucessfully created
    if (pid == 0) {
        if (isInOrOut == 0) {
            dup2(file, 1);
        }
        else {
            dup2(file,0);
        }
        close(file);
        // execute the command
        execvp(command[0],command);
        // if there is any error in execvp(), report
        perror("bad command\n");
        prevExitcode = 1;
        exit(1);
    }
    // child is unsucessfully created
    else if (pid < 0) {
        perror("Child is unsuccessfully created");
        prevExitcode = 1;
        exit(1);
    }
    // Run in parent process
    else {
        // wait the child process to done
        wait(&status_code);
        close(file);
    }

}

void runBackground(char *args[]) {

}

int main(int argc, char *argv[])
{
    struct sigaction new_action;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_handler = sig_handler;
    new_action.sa_flags = 0;
    /*Ctrl+Z*/
    sigaction(SIGTSTP,&new_action, NULL);
    /*Ctrl+C*/
    sigaction(SIGINT,&new_action, NULL);

    char buffer[MAX_CMD_BUFFER];
    char *args[MAX_CMD_BUFFER / 2];
    char lastCmd[MAX_CMD_BUFFER] = "";
    FILE *file = NULL;

    if (argc == 2)
    {
        file = fopen(argv[1], "r"); // read the file
    }

    while (1)
    {
        int isRedir = 0;
        // If there is a shell script file
        if (file != NULL)
        {
            fgets(buffer, 255, file);
        }
        // if not
        else
        {
            printf("icsh $ ");
            fgets(buffer, 255, stdin);
        }

        // When users give an empty command, your shell just give a new prompt.
        if (strlen(buffer) == 1)
        {
            continue;
        }
        else
        {
            // Remove '\n'
            buffer[strlen(buffer) - 1] = '\0';

            // Double-bang: repeat the last command given to the shell.
            if (strcmp(buffer, "!!") == 0)
            {
                if (strlen(lastCmd) == 0)
                {
                    continue;
                }
                else
                {
                    // Replace the buffer with lastCmd and run it
                    strcpy(buffer, lastCmd);
                    printf("%s\n", buffer);
                }
            }
            // store the buffer into the lastCmd
            else
            {
                strcpy(lastCmd, buffer);
            }

            // Tokenize the buffer
            char *token = strtok(buffer, " ");

            int i = 0;
            while (token != NULL)
            {
                args[i] = token;
                token = strtok(NULL, " ");
                // if the STDIN contains < or > 
                if (strcmp(args[i],">")==0) {
                    isRedir = 1;
                    isInOrOut = 0;
                }
                else if (strcmp(args[i],"<")==0) {
                    isRedir = 1;
                    isInOrOut = 1;
                }
                else if (strcmp(args[i],"&")==0) {
                    isBgjob = 1;
                }
                i++;
            }
            args[i] = NULL;
            char *cmd = args[0];

            // I/O Redirection -- redirect the file
            if (isRedir) {
                reDirect(args);
                isRedir = 0;
            }
            // echo <text> -- the echo command prints a given text
            // (until EOL) back to the console.
            else if (strcmp(cmd, "echo") == 0)
            {
                // echo $?
                if (strcmp(args[1],"$?") == 0) {
                    printf("%d\n",prevExitcode);
                }
                else {
                printString(args);
                }
            }
            // exit <num> -- this command exits the shell with a given exit code.
            else if (strcmp(cmd, "exit") == 0)
            {
                int exitCode = atoi(args[1]);
                // if the exit code is more than 255, truncate into 8 bit
                if (exitCode < 255)
                {
                    exitCode = exitCode >> 8;
                }
                printf("Good bye!\n");
                prevExitcode = exitCode;
                exit(exitCode);
            }
            // run foregroundJob -- running external program
            else
            {
                runForeground(args);
            }
        }
    }
}
