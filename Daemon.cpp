#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <utmp.h>
#include <err.h>
#include <string.h>
using namespace std;
#define NAME_WIDTH  8

FILE *file(char *name)
    {
        FILE *ufp;

        if (!(ufp = fopen(name, "r"))) {
            err(1, "%s", name);
        }
        return(ufp);
    }



    
    

void *checkUsers(void *);
void *checkCondition(void *);
void *checkCPUUsage(void *);
FILE *daemonLog;
#define LIMITE 10000
bool primo[LIMITE + 5];




static void criba(){
	for(int i= 0;i <= LIMITE; primo[i++]=true);
	primo[0] = primo[1] = 0;
	FILE *primes;
	primes = fopen("/tmp/primes.out","w");
	for(int i=2; i*i <= LIMITE; i++){
		if(primo[i]){
			fprintf(primes, "%d\n", i);
			for(int j=i*i;j <= LIMITE; j += i){
				primo[j] = 0;
			}
		}
	}
	fclose(primes);
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

	criba();
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
		FILE *ufp;
	    


		struct tm *tiempo;
		time_t tim;

		time(&tim);
		tiempo = localtime(&tim);
		char cadena[128];
		strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);

		FILE *userLog;
		userLog = fopen("/tmp/userlog.out","a");
	
		fprintf(userLog,"[%s] -> CHeking for users\n", cadena);
		struct utmp ut_entry;
		FILE    *fp = fopen(UTMP_FILE, "r");

		if( !fp )
		{
		  printf("Could not open utmp file!");
		}
		int n = 100;

		while(fread(&ut_entry, sizeof(struct utmp), 1, fp) == 1)
		{
		    if(ut_entry.ut_type != USER_PROCESS)
		        continue;

		    // string entries are not 0 terminated if too long...
		    // copy user name to make sure it is 0 terminated

		    char tmpUser[UT_NAMESIZE+1] = {0};
		    strncpy(tmpUser, ut_entry.ut_user, UT_NAMESIZE);
		   	fprintf(userLog, "user ->>> %s\n", tmpUser);
		}


		fclose(userLog);
		sleep (10);
	}
}
void *checkCondition(void *parameters){
	FILE *conditionslog;
		conditionslog = fopen("/tmp/conditionslog.out","a");
	for(int i= 0;i < LIMITE; i++){
		if(primo[i]){
			fprintf(conditionslog, "%d\n", i);
		}
	}
	fclose(conditionslog);
	while(1){
		struct tm *tiempo;
		time_t tim;
		srand(time(NULL));
		int r = rand() % LIMITE;
		time(&tim);
		tiempo = localtime(&tim);
		char cadena[128];
		strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);
		FILE *conditionslog;
		conditionslog = fopen("/tmp/conditionslog.out","a");
		fprintf(conditionslog, "Numero encontrado: %d\n",r);
		if(primo[r]){
			fprintf(conditionslog,"[%s] -> CHeking for conditions trueeeee\n",cadena);
		}else{
			fprintf(conditionslog,"[%s] -> CHeking for conditions falseeeeee\n",cadena);
		}
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