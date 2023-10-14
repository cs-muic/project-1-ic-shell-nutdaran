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
#define MAX_CMD_CHAR 256
#define MAX_JOB 100
#define MAX_CMD 10
#define MAX_PID 4194304 //64bit system

pid_t pid;
pid_t foregroudPID; // foreground process id
int isInOrOut = -1;
int status_code;
int isBgjob = 0;
int cur = 1;
int curJob;
int prevJob;
int prev_exit_code = 0;
char bufferCopy[MAX_CMD_BUFFER] = "";

void createJob(pid_t);
char getSign(int);

/*
    job Status code:
    0 => Running bg
    1 => Running fg
    2 => Stopped
    */
typedef struct {
    int id;
    char command[MAX_CMD_CHAR];
    pid_t PID;
    int jobStatus;
} job;

int listOfJob[MAX_JOB];
job pidJobList[MAX_PID];

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

// External job
void runExternal(char *args[])
{
    pid = fork();
    // child is sucessfully created
    if (pid == 0)
    {
        status_code = execvp(args[0], args);
        if (status_code == -1) {
            printf("bad command\n");
        }
        exit(1);
    }
    // child is unsuccessfully created
    else if (pid < 0)
    {
        perror("Child is unsuccessfully created");
        exit(1);
    }
    // Run in parent process
    else
    {
        foregroudPID = pid;
        // Wait until the foreground process finished
        waitpid(pid, &status_code, 0);
        foregroudPID = 0;
        prev_exit_code = WEXITSTATUS(status_code);
    }
}

void runBackground(char *args[]) {
    pid = fork();
    // child is sucessfully created
    if (pid == 0)
    {
        status_code = execvp(args[0], args);
        if (status_code == -1) {
            printf("bad command\n");
        }
        exit(1);
    }
    // child is unsuccessfully created
    else if (pid < 0)
    {
        perror("Child is unsuccessfully created");
        exit(1);
    }
    // Run in parent process
    else
    {
        createJob(pid);
    }
    isBgjob = 0;
}

// BG child handler
void child_handler(int sig, siginfo_t *sip, void *notused) {
    int status = 0;
    if (sip->si_pid == waitpid(sip->si_pid,&status, WNOHANG)) {
        if (WIFEXITED(status)|| WTERMSIG(status)) {
            job currentJob = pidJobList[sip->si_pid];
            if (currentJob.jobStatus == 0 && currentJob.id)
            {
                char* cmd = currentJob.command;
                cmd[strlen(currentJob.command) - 1] = '\0';
                printf("\n[%d]  %c %d done       %s\n", currentJob.id,getSign(currentJob.id),sip->si_pid,cmd);
                fflush(stdout);
                listOfJob[currentJob.id - 1] = 0;
                job emptyJob;
                emptyJob.id = 0;
                emptyJob.PID = 0;
                pidJobList[pid] = emptyJob;
                // printf("%d",listOfJob[currentJob.id]);
            }
        }
    }
}

// SIGNAL HANDLER
void sig_handler(int signum) {
    /*Ctrl+Z*/
    if (signum == SIGTSTP && foregroudPID) {
        printf("PID: %d. Foreground process is stop(suspended).\n",foregroudPID);
        kill(foregroudPID, SIGTSTP);
    }
    /*Ctrl+C*/
    else if (signum == SIGINT && foregroudPID) {
        printf("PID: %d. Foreground process is killed.\n",foregroudPID);
        kill(foregroudPID, SIGINT);
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
    char *command[MAX_CMD];
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
            exit (errno);
        }
    }
    // Output
    else if (isInOrOut==0){
        file = open (fileName, O_TRUNC | O_CREAT | O_WRONLY, 0666);
        if (file < 0) {
            fprintf (stderr, "Couldn't open a file\n");
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
        exit(1);
    }
    // child is unsucessfully created
    else if (pid < 0) {
        perror("Child is unsuccessfully created");
        exit(1);
    }
    // Run in parent process
    else {
        // wait the child process to done
        wait(&status_code);
        close(file);
    }
}

void createJob(pid_t pid) {
    job new_job;
    strcpy(new_job.command,bufferCopy);
    new_job.jobStatus = 0;
    new_job.PID = pid;
    prevJob = curJob;
    
    for (int i = 0; i < cur; i++) {
        if (listOfJob[i] == 0) {
            new_job.id = i+1;
            curJob = i+1;
            listOfJob[i] = pid;
            break;
        }
        else if ((i+1) == cur) {
            new_job.id = cur;
            curJob = cur;
            ++cur;
        }
    }
    pidJobList[pid] = new_job;
    printf("[%d] %d\n",new_job.id,new_job.PID);
}

char* getStatus(int status) {
    switch (status) {
        case 0:
            return "Running";
        case 2:
            return "Stoppped";
        default:
            return " ";
    }
}

char getSign(int pos) {
    if (pos == curJob) { // current job
        return '+';
    }
    else if (pos == prevJob) { // previous job
        return '-';
    }
    else { // old jobs
        return ' ';
    }
}

void getJobList() {
    int count = 0;
    if (cur == 0) {
        printf("--------- No Background jobs ---------\n");
    }
    else {
        for (int i = 0; i < cur; i++)
            {
                int pid = listOfJob[i];
                job Job = pidJobList[pid];
                // printf("%d",pid);
                if (Job.PID != 0) {
                    printf("[%d]%c  %s                 %s\n",Job.id,getSign(Job.id),getStatus(Job.jobStatus),Job.command);
                    ++count;
                }
            }
        if (count == 0) {
            printf("--------- No Background jobs ---------\n");
        }
    }
}

void printHelp() {
    printf("#############################################\n");
    printf("#     COMMAND     |        Description      #\n");
    printf("#############################################\n");
    printf("# echo <text>     |  print the line         #\n");
    printf("# exit <num>      |  quit the shell         #\n");
    printf("# !!              |  run the last cmd       #\n");
    printf("# <file.sh>       |  execute script file    #\n");
    printf("# <cmd>           |  run Foreground job     #\n");
    printf("# ctrl+z          |  stop Foreground job    #\n");
    printf("# ctrl+c          |  kill Foreground job    #\n");
    printf("# <cmd> &         |  run Background job     #\n");
    printf("# <cmd> > <file>  |  redirect output to file#\n");
    printf("# <cmd> < <file>  |  take input from file   #\n");
    printf("#############################################\n");
}

void init_handler() {
    struct sigaction new_action;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_handler = sig_handler;
    new_action.sa_flags = 0;
    /*Ctrl+Z*/
    sigaction(SIGTSTP,&new_action, NULL);
    /*Ctrl+C*/
    sigaction(SIGINT,&new_action, NULL);

    struct sigaction action;
    action.sa_sigaction = child_handler; /* Note use of sigaction, not handler */
    sigfillset (&action.sa_mask);
    action.sa_flags = SA_SIGINFO; /* Note flag,otherwise NULL in function*/
    sigaction (SIGCHLD, &action, NULL);
}

int main(int argc, char *argv[])
{
    init_handler();

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
        // printf("%s",buffer);
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
                    // printf("%s\n", buffer);
                }
            }
            // store the buffer into the lastCmd
            else
            {
                strcpy(lastCmd, buffer);
                strcpy(bufferCopy,buffer);
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
                    break;
                }
                else if (strcmp(args[i],"<")==0) {
                    isRedir = 1;
                    isInOrOut = 1;
                    break;
                }
                else if (strcmp(args[i],"&")==0) {
                    isBgjob = 1;
                    args[i] = NULL; //remove &
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
                    printf("%d\n",0);
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
                exit(exitCode);
            }
            // Banckground job
            else if (isBgjob) {
                runBackground(args);
            }
            else if (strcmp(cmd, "jobs") == 0) {
                getJobList();
            }
            else if (strcmp(cmd,"fg")==0) {

            }
            else if (strcmp(cmd,"bg")==0) {

            }
            else if (strcmp(cmd,"help")==0){
                printHelp();
            }
            // run foregroundJob -- running external program
            else
            {
                runExternal(args);
            }
        }
    }
}
