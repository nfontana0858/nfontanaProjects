/*
Name: Nicholas Fontana
Pledge: I pledge my honor that I have abided by the Stevens Honor System
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"
#define MAX_ARGS 64
#define INPUT_BUFFER_SIZE 1024

volatile sig_atomic_t interrupted = 0;

void handle_sigint(int sig) {
    interrupted = 1;
}


void print_prompt(void){
    char cwd[PATH_MAX]; // Defines buffer

    if (getcwd(cwd, sizeof(cwd)) != NULL) { //Attempts to get working directory in buffer
        printf("%s[%s]> %s", BLUE, cwd, DEFAULT); // prints BLUE if it works then resets to DEFAULT

    } else { //Prints error if buffer fails
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}


void handle_cd(char **args) {
    char *target_directory;
    if (args[1] != NULL && args[2] != NULL) {
        fprintf(stderr, "Error: Too many arguments to cd.\n");
        return;
    }

    if (args[1] == NULL || strcmp(args[1], "~") == 0) {
        uid_t uid = getuid();
        struct passwd *pw_entry = getpwuid(uid);
        if (pw_entry == NULL) {
            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        target_directory = pw_entry->pw_dir;
    } else {
        target_directory = args[1];
    }

    if (chdir(target_directory) != 0) {
        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", target_directory, strerror(errno));
    }
}

void handle_exit(char **args){
    int exit_status;
    
    if(args[1] != NULL){
        exit_status = atoi(args[1]);
    } else {
        exit_status = EXIT_SUCCESS;
    }

    exit(exit_status);
}

void handle_pwd(char **args){
    char cwd[PATH_MAX]; 
    if(getcwd(cwd, sizeof(cwd)) != NULL){
        printf("%s\n", cwd);
    }else{
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
    }
}

void handle_lf(char **args){
    DIR *dir = opendir(".");
    struct dirent *entry; 
    
    if (dir == NULL) {
        fprintf(stderr, "Error: Unable to open directory. %s\n", strerror(errno));
        return 1;
    }
    
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return;

}


void handle_lp(char **args) {
    typedef struct {
        int pid;
        char username[256];
        char cmdline[256];
    } proc_info;

    int compare_by_pid(const void *a, const void *b) {
        const proc_info *pa = (const proc_info *)a;
        const proc_info *pb = (const proc_info *)b;
        return (pa->pid - pb->pid);
    }

    DIR *dir = opendir("/proc");  
    if (dir == NULL) {
        fprintf(stderr, "Error: Unable to open directory. %s\n", strerror(errno));
        return;
    }

    size_t capacity = 128;
    size_t size = 0;
    proc_info *plist = malloc(capacity * sizeof(proc_info));
    if (plist == NULL) {
        fprintf(stderr, "Error: Malloc failed.\n");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            char status_file[256];
            snprintf(status_file, sizeof(status_file), "/proc/%d/status", pid);

            FILE *fp = fopen(status_file, "r");
            if (fp == NULL)
                continue;  

            char line[256];
            int uid = -1;
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (sscanf(line, "Uid:\t%d", &uid) == 1) {
                    break;
                }
            }
            fclose(fp);

            struct passwd *pw = getpwuid(uid);
            if (pw != NULL) {
                char cmdline_file[256];
                snprintf(cmdline_file, sizeof(cmdline_file), "/proc/%d/cmdline", pid);

                fp = fopen(cmdline_file, "r");
                if (fp != NULL) {
                    char cmdline[256] = {0};
                    if (fgets(cmdline, sizeof(cmdline), fp) != NULL) {
                        if (size == capacity) {
                            capacity *= 2;
                            proc_info *temp = realloc(plist, capacity * sizeof(proc_info));
                            if (temp == NULL) {
                                fprintf(stderr, "Error: Realloc failed.\n");
                                fclose(fp);
                                free(plist);
                                closedir(dir);
                                return;
                            }
                            plist = temp;
                        }
                        plist[size].pid = pid;
                        strncpy(plist[size].username, pw->pw_name, sizeof(plist[size].username) - 1);
                        strncpy(plist[size].cmdline, cmdline, sizeof(plist[size].cmdline) - 1);
                        plist[size].username[sizeof(plist[size].username) - 1] = '\0';
                        plist[size].cmdline[sizeof(plist[size].cmdline) - 1] = '\0';
                        size++;
                    }
                    fclose(fp);
                }
            }
        }
    }

    closedir(dir);

    qsort(plist, size, sizeof(proc_info), compare_by_pid);

    for (size_t i = 0; i < size; i++) {
        printf("%d %s %s\n", plist[i].pid, plist[i].username, plist[i].cmdline);
    }

    free(plist);
}



void parse_input(char *input, char **args) {
    int i = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = SA_RESTART; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    char input[INPUT_BUFFER_SIZE];
    char *args[MAX_ARGS];

    while (1) {
        if (interrupted) {
            printf("\n");
            interrupted = 0;
            continue;
        }
        
        print_prompt();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        parse_input(input, args);
        if (args[0] == NULL)
            continue;
        
        if (strcmp(args[0], "cd") == 0) {
            handle_cd(args);
        } else if (strcmp(args[0], "exit") == 0) {
            handle_exit(args);
        } else if (strcmp(args[0], "pwd") == 0) {
            handle_pwd(args);
        } else if (strcmp(args[0], "lf") == 0) {
            handle_lf(args);
        } else if (strcmp(args[0], "lp") == 0) {
            handle_lp(args);
        } else {
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
            } else if (pid == 0) {

                if (execvp(args[0], args) == -1) {
                    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            } else {
                int status;
                if (wait(&status) == -1) {
                    fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                }
            }
        }
    }
    return 0;
}