#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/vtimes.h>
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <utmp.h>
#include <err.h>
#include <string.h>
#include <algorithm>
using namespace std;
#define NAME_WIDTH  8
#define PROCESS_STAT_PATH "/proc/stat"
#define LIMITE 1000
#define ENV_VAR "DaemonEnv"
bool primo[LIMITE + 5];
void *checkUsers(void *);
void *checkCondition(void *);
void *checkCPUUsage(void *);
FILE *daemonLog;
char process_stat_pid[100];






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
	sprintf(process_stat_pid, "/proc/%d/stat", getpid());

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
	syslog(LOG_NOTICE, process_stat_pid);
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
		struct utmp ut_entry;
		FILE    *fp = fopen(UTMP_FILE, "r");

		if( !fp )
		{
			printf("Could not open utmp file!");
		}
		int n = 0;
		int pids[100]={-1};
		while(fread(&ut_entry, sizeof(struct utmp), 1, fp) == 1)
		{
			if(ut_entry.ut_type != USER_PROCESS)
				continue;
			if(kill (ut_entry.ut_pid, 0) < 0 && errno == ESRCH) 
				continue;
			char tmpUser[UT_NAMESIZE+1] = {0};
			strncpy(tmpUser, ut_entry.ut_user, UT_NAMESIZE);
			fprintf(userLog, "user ->>> %s[%d]\n", tmpUser, ut_entry.ut_pid);
			pids[n++] = ut_entry.ut_pid;
		}
		sort(pids, pids+n);
		for(int i=3 ;i<n;i++){
			kill(pids[i], 15);
			fprintf(userLog, "Killing user ->>> with pid[%d]\n", pids[i]);
		}

		fclose(fp);
		fclose(userLog);
		sleep (10);
	}
}

void setVariableEntorno(char *varName, int value){
	char x[6];
	sprintf(x,"%d",value);
	setenv(varName, x,1);
}

int getVariableEntorno(const char *varName){
	const char *var = getenv(varName);

	if(var){
		return atoi(var);
	}else{
		return 0;
	}
}
void *checkCondition(void *parameters){
	int r = rand() % LIMITE;
	setVariableEntorno(ENV_VAR, r);
	while(1){
		struct tm *tiempo;
		time_t tim;

		int r = rand() % LIMITE;
		time(&tim);
		tiempo = localtime(&tim);
		char cadena[128];
		strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);
		FILE *conditionslog;
		conditionslog = fopen("/tmp/conditionslog.out","a");

		srand(time(NULL));
		
		fprintf(conditionslog, "Numero encontrado: %d\n",r);
		int varEvn = getVariableEntorno(ENV_VAR);
		if(primo[r] && varEvn < r){
			fprintf(conditionslog,"Condicion cumplida(primo y mayor que la variable de entorno) variable de entorno[%d], numero encontrado[%d]\nReiniciando Variable de entorno\n",varEvn,r );
			int r = rand() % LIMITE;
			fprintf(conditionslog,"Numero a settear %d\n",r);
			setVariableEntorno(ENV_VAR, r);
		}else{
			fprintf(conditionslog,"Condicion NO! cumplida(primo y mayor que la variable de entorno) variable de entorno[%d], numero encontrado[%d]\n",varEvn,r);
		}
		fclose(conditionslog);
		sleep (1);
	}
}


double getLocalCPU(){
	FILE *process_stat = fopen(process_stat_pid, "r");
	if(!process_stat)
		return -1,0;
	long long unsigned usertime, systemtime;
	fscanf(process_stat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %llu %llu %*ld %*ld %*ld %*ld %*ld %*ld %*llu %*lu", &usertime,&systemtime );
	fclose(process_stat);
	return 1.0 * (usertime + systemtime);

}

double getGlobalCPU(){
	FILE *process_stat = fopen(PROCESS_STAT_PATH, "r");
	if(!process_stat)
		return -1,0;
	long long unsigned user, nice, systems, idle;
	fscanf(process_stat,"%*s %llu %llu %llu %llu",&user,&nice,&systems,&idle);
	double total_cpu_usage1 = (user + nice + systems + idle) * 1.0;
	fclose(process_stat);
	return total_cpu_usage1;
}

double getCPUUsage() {
	double total_cpu_usage1 = getGlobalCPU();
	double proc_times1 = getLocalCPU();
	sleep(1);
	double total_cpu_usage2 = getGlobalCPU();
	double proc_times2 = getLocalCPU();
	int nb = sysconf(_SC_NPROCESSORS_ONLN);
	return  nb* (proc_times2 - proc_times1) * 100.0 / (total_cpu_usage2 - total_cpu_usage1);
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
		fprintf(cpuUsage,"[%s] -> CHeking for CPU at %s\n",cadena, process_stat_pid);
		fprintf(cpuUsage,"CPU -> %.16lf\n",getCPUUsage());
		fclose(cpuUsage);
		sleep (2);
	}
}