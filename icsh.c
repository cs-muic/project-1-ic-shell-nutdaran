/* ICCS227: Project 1: icsh
 * Name: Daran Thawornwattanapol
 * StudentID: 6380413
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

#define MAX_CMD_BUFFER 255

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

void runForground(char *args[])
{
    char *prog_argv[4];
    // create child process
    pid_t pid = fork();

    for (int i = 0; i < 4; i++)
    {
        if (args[i] != NULL)
        {
            prog_argv[i] = args[i];
        }
        else
        {
            prog_argv[i] = NULL;
        }
    }

    if (pid < 0) //child is unsuccessful
    {
        perror("Fork failed");
        exit(1);
    }
    else if (pid == 0) // child is sucessfully created
    {
        execvp(prog_argv[0], prog_argv);
    }
    else // return to parent process
    {
        waitpid(pid, NULL, 0);
    }
}

int main(int argc, char *argv[])
{
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
            else
            {
                // store the buffer into the lastCmd
                strcpy(lastCmd, buffer);
            }

            // Tokenize the buffer
            char *token = strtok(buffer, " ");
            int i = 0;
            while (token != NULL)
            {
                args[i] = token;
                token = strtok(NULL, " ");
                i++;
            }
            args[i] = NULL;
            char *cmd = args[0];

            // echo <text> -- the echo command prints a given text
            // (until EOL) back to the console.
            if (strcmp(cmd, "echo") == 0)
            {
                printString(args);
            }
            // exit <num> -- this command exits the shell with a given exit code.
            else if (strcmp(cmd, "exit") == 0)
            {
                int exitCode = atoi(args[1]);
                if (exitCode >= 0 && exitCode <= 255)
                {
                    printf("Good bye!\n");
                    exit(exitCode);
                }
                // if the exit code is more than 255, truncate into 8 bit
                else
                {
                    exitCode = exitCode >> 8;
                    printf("Good bye!\n");
                    exit(exitCode);
                }
            }
            else
            {
                runForground(args);
            }
        }
    }
}
