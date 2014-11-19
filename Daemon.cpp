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





/**
* Saca todos los primos entre 2 y el limite
*/
static void criba(){
	for(int i= 0;i <= LIMITE; primo[i++]=true);
	primo[0] = primo[1] = 0;
	for(int i=2; i*i <= LIMITE; i++){
		if(primo[i]){
			for(int j=i*i;j <= LIMITE; j += i){
				primo[j] = 0;
			}
		}
	}
}

static void skeleton_daemon()
{
	pid_t pid; //PID del proceso

	struct stat st;
	pthread_t threadUsers, threadConditions, threadCPU; //Hilos del demonio
	pid = fork(); //Hace un fork al proceso actual

	if (pid < 0) //Un error al realizar el fork
		exit(EXIT_FAILURE);

	
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid > 0)
		exit(EXIT_SUCCESS);

	//Brindar nuevos permisos al proceso
	umask(0);

	//Cambia el directorio para uno más seguro
	chdir("/");

	//Cierra todos los logs que halla abiertos
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
	{
		close (x);
	}
	
	criba(); //Ejecuta el metodo para caultar primos
	int error;
	sprintf(process_stat_pid, "/proc/%d/stat", getpid()); //Imprime en la variable process_stat_pid la ruta /proc/<PID>/stat para luego ser utilizada al calcular la CPU

	error = pthread_create(&threadUsers, NULL, &checkUsers, NULL) ;  //Se crea el hilo de usuarios con la función a ejecutarse checkusers
	if(error != 0) {
		syslog( LOG_ERR, "No se pudo crear el hilo" );
		exit( EXIT_FAILURE );
	}
	error = pthread_create(&threadConditions, NULL, &checkCondition, NULL) ;  //Se crea el hilo de variables de entorno con la función a ejecutarse checkCondition
	if(error != 0) {
		syslog( LOG_ERR, "No se pudo crear el hilo" );
		exit( EXIT_FAILURE );
	}
	error = pthread_create(&threadCPU, NULL, &checkCPUUsage, NULL) ;  //Se crea el hilo de contrl de CPU con la función a ejecutarse checkCPUUsage
	if(error != 0) {
		syslog( LOG_ERR, "No se pudo crear el hilo" );
		exit( EXIT_FAILURE );
	}
	openlog ("firstdaemon", LOG_PID, LOG_DAEMON); //abre un log /var/sys/log

}

int main()
{
	
	skeleton_daemon(); //Vuelve el programa un demonio
	syslog (LOG_NOTICE, "First daemon started."); //Imprime en los logs
	syslog(LOG_NOTICE, process_stat_pid); //Logs
	while (1) //Ciclo infinito para que nunca muera
	{
		sleep(1); //Duerme el demonio por 1 segundo
	}

	syslog (LOG_NOTICE, "First daemon terminated.");
	closelog();

	return EXIT_SUCCESS;
}


/*Funcion para checkear los usuarios */
void *checkUsers(void *parameters){
	while(1){
		struct tm *tiempo; //estuctura para guardar el tiempo actual
		time_t tim;
		time(&tim);
		tiempo = localtime(&tim);
		char cadena[128];
		strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo); //Formatea la fecha actual para que sea entendible por el usuario

		FILE *userLog; //Crea un puntuero a un archivo
		userLog = fopen("/tmp/userlog.out","a"); //Abre el archivo /tmp/userLog.out con la opcoin 'a' (append), si no existe lo crea
		fprintf(userLog,"[%s] -> CHeking for users\n", cadena); //Imprime en el archivo userLog.out
		struct utmp ut_entry; //Se crea la estructura utmp que son los registros de los usuarios loggeados en el sistema
		FILE    *fp = fopen(UTMP_FILE, "r"); //Se abre el archivo donde se encuentran lso datos de los usuarios

		if( !fp )
		{
			printf("Could not open utmp file!");
		}
		int n = 0;
		int pids[100]={-1};
		while(fread(&ut_entry, sizeof(struct utmp), 1, fp) == 1)//Mientras halla mas lineas en el archivo de utmp haga
		{

			if(ut_entry.ut_type != USER_PROCESS) //Si no es un proceso de usuario se salta la entrada
				continue;
			if(kill (ut_entry.ut_pid, 0) < 0 && errno == ESRCH) //Si el proceso esta muerto salta la entrada
				continue;
			char tmpUser[UT_NAMESIZE+1] = {0};//Crea un puntero a un string
			strncpy(tmpUser, ut_entry.ut_user, UT_NAMESIZE);//Llena el string con el nombre de usuario
			fprintf(userLog, "user ->>> %s[%d]\n", tmpUser, ut_entry.ut_pid);
			pids[n++] = ut_entry.ut_pid; //Guarda los pids de los usuarios loggeados
		}
		sort(pids, pids+n); //Organiza los PIDS de menor a mayor 
		for(int i=3 ;i<n;i++){//Si hay mas de tres usuarios mata los procesos de esos usuarios
			kill(pids[i], 15);//hace el kill del proceso de loggeo del usuario
			fprintf(userLog, "Killing user ->>> with pid[%d]\n", pids[i]); //imprime en el log
		}

		fclose(fp); //cierra los archivos para que se haga el flush
		fclose(userLog); 
		sleep (10); //Duerme el hilo por 10 segundos
	}
}

/*Establece la variable de entorno al valor que se desea, si no existe la crea*/
void setVariableEntorno(char *varName, int value){
	char x[6];
	sprintf(x,"%d",value);
	setenv(varName, x,1);//Funcion para crear o modificar variable de entorno
}


/*Recupera el valor de la varible de entorno*/
int getVariableEntorno(const char *varName){
	const char *var = getenv(varName);

	if(var){
		return atoi(var);
	}else{
		return 0;
	}
}

/*Hilo para probar la condicion*/
void *checkCondition(void *parameters){
	int r = rand() % LIMITE; //Hace un random de 0 al Limite
	setVariableEntorno(ENV_VAR, r); //Establece la variable de entorno al valor del numero random
	while(1){//Ciclo infinito
		struct tm *tiempo;
		time_t tim;

		int r = rand() % LIMITE;
		time(&tim);
		tiempo = localtime(&tim);
		char cadena[128];
		strftime(cadena, 128, "%Y:%m:%d%H:%M:%S", tiempo);
		FILE *conditionslog;
		conditionslog = fopen("/tmp/conditionslog.out","a");//Abre el archivo conditionslog.out que se encuentra en /tmp/conditionsLog

		srand(time(NULL));//Cambia la semilla del random
		
		fprintf(conditionslog, "Numero encontrado: %d\n",r); //Imprime el numero random que se acabade hallar
		int varEvn = getVariableEntorno(ENV_VAR); //Recupera el valor de la variable de entorno
		if(primo[r] && varEvn < r){//Si el numero R es un primo y es mayor que la variable de entorno, se estable un nuevo valor y se imprime en log
			fprintf(conditionslog,"Condicion cumplida(primo y mayor que la variable de entorno)\nvariable de entorno[%d], numero encontrado[%d]\nReiniciando Variable de entorno\n",varEvn,r );
			int r = rand() % LIMITE;
			fprintf(conditionslog,"Numero a establecer %d\n-----------------------------------------------------------\n",r);
			setVariableEntorno(ENV_VAR, r);
		}else{
			fprintf(conditionslog,"No se cumple la condicion(primo y mayor que la variable de entorno)\nvariable de entorno[%d], numero encontrado[%d]\n-----------------------------------------------------------\n",varEvn,r);
		}
		fclose(conditionslog);//Cierra el archivo
		sleep (2);//Duerme el hilo por 2 segundos
	}
}

//Devuelve el tiempo de CPU del proceso actual
double getLocalCPU(){
	FILE *process_stat = fopen(process_stat_pid, "r");//Lee el archivo de /proc/<PID>/stats donde se encuentra los tiempos y el uso
	if(!process_stat)
		return -1,0;
	long long unsigned usertime, systemtime;
	/*Escanea la primera linea del archivo donde los datos relevantes*/
	fscanf(process_stat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %llu %llu %*ld %*ld %*ld %*ld %*ld %*ld %*llu %*lu", &usertime,&systemtime );
	fclose(process_stat);//Ciera el archivo
	return 1.0 * (usertime + systemtime); //Devuelve la suma del tiempo de usuario y tiempo de sistema como un decimal

}

/*Devuelve el timpo de CPU del todo el sistema*/
double getGlobalCPU(){
	FILE *process_stat = fopen(PROCESS_STAT_PATH, "r"); //Abre el archivo /proc/stats
	if(!process_stat)
		return -1,0;
	long long unsigned user, nice, systems, idle;
	fscanf(process_stat,"%*s %llu %llu %llu %llu",&user,&nice,&systems,&idle);
	double total_cpu_usage1 = (user + nice + systems + idle) * 1.0; //Calcula el tiempo del sistema
	fclose(process_stat);
	return total_cpu_usage1; //Devuelve el uso de cpu 
}

//Calcula el porcentage de uso de CPU
double getCPUUsage() {
	double total_cpu_usage1 = getGlobalCPU();
	double proc_times1 = getLocalCPU();
	sleep(1);//Calcula datos espera un segundo  y vuelve a calcular datos
	double total_cpu_usage2 = getGlobalCPU();
	double proc_times2 = getLocalCPU();
	int nb = sysconf(_SC_NPROCESSORS_ONLN);//Numero de procesadores
	return  nb* (proc_times2 - proc_times1) * 100.0 / (total_cpu_usage2 - total_cpu_usage1); //Calcula el USO de CPU
}

/*Calcula el uso de CPU*/
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