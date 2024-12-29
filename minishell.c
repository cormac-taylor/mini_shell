/*******************************************************************************
 * Name        : minishell.c
 * Author      : Cormac Taylor
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t interrupted = 0;

int cmp (const void * a, const void * b) {
   return (*(int*) a - *(int*) b);
}

char* tostring(int num){

    int len = 0;
    int temp = num;

    if(num == 0){
        len = 1;
    }
    while (temp != 0){
        len++;
        temp /= 10;
    }

    char* res = (char*) malloc(len + 1);
    int rem;
    for (int i = 0; i < len; i++){
        rem = num % 10;
        num = num / 10;
        res[len - (i + 1)] = rem + '0';
    }
    res[len] = '\0';
    return res;
}


char* concat(char* beg, char* end){
    
    char* res = (char*) malloc(strlen(beg)+strlen(end)+1);
    if(res == NULL){
        return NULL;
    }

    int i = 0;
    while(beg[i] != '\0'){
        *((char*)res + i) = beg[i];
        i++;
    }
    int j = 0;
    while(end[j] != '\0'){
        *((char*)res + i) = end[j];
        i++;
        j++;
    }
    *((char*)res + i) = '\0';
    return res;
}

//review
void signalHandler(int s) {

    interrupted = 1;
    printf("\n");

}

int main(int argc, char* argv[]){

    char cwd[PATH_MAX];
    char cmd_line[PATH_MAX];
    char cmd_line_cpy[PATH_MAX];

    struct sigaction action = {0};
    action.sa_handler = signalHandler;
    sigaction(SIGINT, &action, NULL);

    while(1){

        char* ret = getcwd(cwd, PATH_MAX);
        if(ret == NULL){
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            continue;
        }

        printf("%s[%s]%s> ", BLUE, ret, DEFAULT);

        ret = fgets(cmd_line, PATH_MAX, stdin);
        if(ret == NULL){
            if(!interrupted){
                fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
            } else {
                cmd_line[0] = '\0';
            }
            continue;
        }


        if(cmd_line[0] != '\n'){

            //get rid of '\n'
            int i = 0;
            while(cmd_line[i] != '\0'){
                if(cmd_line[i] == '\n'){
                    cmd_line[i] = ' ';
                }
                i++;
            }

            //copy cmd_line to cmd_line_cpy
            for(int j = 0; j <= i; j++){
                cmd_line_cpy[j] = cmd_line[j];
            }

            char* arg = strtok(cmd_line, " ");
            int len = 1;

            //count num of args
            while(arg != NULL) {
                len++;
                arg = strtok(NULL, " ");
            }

            char** args = (char**) malloc(len * sizeof(char*));
            if(args == NULL){
                fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
                continue;
            }

            arg = strtok(cmd_line_cpy, " ");

            //initalize args
            for(int j = 0; j < len - 1; j++) {

                args[j] = arg;
                arg = strtok(NULL, " ");

            }
            args[len - 1] = NULL;

            //case checking
            if(strcmp(args[0], "exit") == 0){

                free(args);
                return EXIT_SUCCESS;

            } else if (strcmp(args[0], "cd") == 0){

                if(args[1] != NULL && args[2] != NULL){
                    fprintf(stderr, "Error: Too many arguments to cd.\n");
                    free(args);
                    continue;
                }
                
                int ret;
                if(args[1] == NULL || strcmp(args[1], "~") == 0){

                    struct passwd *pw = getpwuid(getuid());
                    if(pw == NULL){
                        fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                        free(args);
                        continue;
                    }

                    ret = chdir(pw->pw_dir);

                } else {
                    ret = chdir(args[1]);
                }

                if(ret != 0){
                    fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", args[1], strerror(errno));
                }

            } else if (strcmp(args[0], "pwd") == 0){
                
                printf("%s\n", cwd);

            } else if (strcmp(args[0], "lf") == 0){
                
                struct dirent* dp;
                struct stat statbuf;

                DIR* open_dir = opendir(cwd);
                while(open_dir != NULL){

                    dp = readdir(open_dir);
                    if(dp != NULL) {

                        char* file_name = dp->d_name;
                        if(strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0){
                            continue;
                        } else {
                            stat(file_name, &statbuf);
                            mode_t mode = statbuf.st_mode;
                            if(S_ISREG(mode)){
                                printf("%s\n", file_name);
                            }
                        }
                    } else {
                        closedir(open_dir);
                        break;
                    }
                }

            } else if (strcmp(args[0], "lp") == 0){
                
                struct dirent* dp;
                struct stat statbuf;
                struct passwd* ownerInfo;

                int num_p = 0;
                DIR* open_dir = opendir("/proc");
                while(open_dir != NULL){

                    dp = readdir(open_dir);
                    if(dp != NULL){
                        char* file_name = dp->d_name;
                        if(strcspn(file_name, "1234567890") == 0){
                            num_p++;
                        }
                    } else {
                        closedir(open_dir);
                        break;
                    }
                }

                int* processes = (int*) malloc(num_p * sizeof(int));
                open_dir = opendir("/proc");
                int i = 0;
                while(open_dir != NULL){

                    dp = readdir(open_dir);
                    if(dp != NULL){
                        char* file_name = dp->d_name;
                        if(strcspn(file_name, "1234567890") == 0){

                            processes[i] = atoi(file_name);
                            i++;
                        }
                    } else {
                        closedir(open_dir);
                        break;
                    }
                }

                qsort(processes, num_p, sizeof(int), cmp);

                char* file_name;
                char* path;
                char* path_cmdline;
                char* cmdline;

                for(i = 0; i < num_p; i++){

                    file_name = tostring(processes[i]);
                    path = concat("/proc/", file_name);
                    if(path == NULL){
                        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
                        continue;
                    }

                    if(stat(path, &statbuf) == -1){
                        fprintf(stderr, "Error: stat() failed. %s.\n", strerror(errno));
                        free(path);
                        continue;
                    }

                    ownerInfo = getpwuid(statbuf.st_uid);
                    if(ownerInfo == NULL){
                        fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                        free(path);
                        continue;
                    }

                    path_cmdline = concat(path, "/cmdline");
                    free(path);

                    if(path_cmdline == NULL){
                        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
                        continue;
                    }

                    FILE* fp = fopen(path_cmdline , "r");
                    if(fp == NULL) {
                        fprintf(stderr, "Error: fopen() failed. %s.\n", strerror(errno));
                        free(path_cmdline);
                        continue;
                    }
                    free(path_cmdline);

                    cmdline = fgets(cwd, PATH_MAX, fp);
                    if(cmdline == NULL){
                        cmdline = "";
                    }

                    printf("%s\t %s %s\n", file_name, ownerInfo->pw_name, cmdline);
                    free(file_name);
                    fclose(fp);
                }

                free(processes);

            } else {
                
                pid_t pid = fork();

                if (pid == -1){

                    fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));

                } else if (pid == 0){

                    //child
                    execvp(args[0], args);
                    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                    free(args);
                    return EXIT_FAILURE;

                } else {

                    //parent
                    int status;
                    if(waitpid(pid, &status, 0) == -1){
                        if(!interrupted){
                            fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                        } else {
                            free(args);
                                    interrupted = 0;
                            continue;
                        }
                    }
                }
            }
            free(args);
        }
        interrupted = 0;
    }

    return EXIT_SUCCESS;

}
