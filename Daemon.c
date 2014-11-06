#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
void *checkUsers(void *);
void *checkCondition(void *);
void *checkCPUUsage(void *);
FILE *daemonLog;



static void criba(){

}

static void skeleton_daemon()
{
    pid_t pid;
    
    struct stat st;
    pthread_t threadUsers, threadConditions, threadCPU;
    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        close (x);
    }


    int error;
    error = pthread_create(&threadUsers, NULL, &checkUsers, NULL) ;  
    if(error != 0) {
            syslog( LOG_ERR, "No se pudo crear el hilo" );
            exit( EXIT_FAILURE );
    }
    error = pthread_create(&threadConditions, NULL, &checkCondition, NULL) ;  
    if(error != 0) {
            syslog( LOG_ERR, "No se pudo crear el hilo" );
            exit( EXIT_FAILURE );
    }
    error = pthread_create(&threadCPU, NULL, &checkCPUUsage, NULL) ;  
    if(error != 0) {
            syslog( LOG_ERR, "No se pudo crear el hilo" );
            exit( EXIT_FAILURE );
    }
    /* Open the log file */
    openlog ("firstdaemon", LOG_PID, LOG_DAEMON);
}
int main()
{
    skeleton_daemon();
    syslog (LOG_NOTICE, "First daemon started.");
    while (1)
    {
        sleep(1);
    }

    syslog (LOG_NOTICE, "First daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}



void *checkUsers(void *parameters){
    while(1){
        struct tm *tiempo;
        time_t tim;

        time(&tim);
        tiempo = localtime(&tim);
        char cadena[128];
        strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);

        FILE *userLog;
        userLog = fopen("/tmp/userlog.out","a");
        fprintf(userLog,"[%s] -> CHeking for users\n", cadena);
        fclose(userLog);
        sleep (10);
    }
}
void *checkCondition(void *parameters){
    while(1){
        struct tm *tiempo;
        time_t tim;

        time(&tim);
        tiempo = localtime(&tim);
        char cadena[128];
        strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);
        FILE *conditionslog;
        conditionslog = fopen("/tmp/conditionslog.out","a");
        fprintf(conditionslog,"[%s] -> CHeking for conditions\n",cadena);
        fclose(conditionslog);
        sleep (5);
    }
}
void *checkCPUUsage(void *parameters){
    while(1){
        struct tm *tiempo;
        time_t tim;

        time(&tim);
        tiempo = localtime(&tim);
        char cadena[128];
        strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);
        FILE *cpuUsage;
        cpuUsage = fopen("/tmp/cpuUsage.out","a");
        fprintf(cpuUsage,"[%s] -> CHeking for CPU\n",cadena);
        fclose(cpuUsage);
        sleep (1);
    }
}