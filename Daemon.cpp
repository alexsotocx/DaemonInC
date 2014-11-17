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
		int n = 0;
		int pids[100]={-1};
		while(fread(&ut_entry, sizeof(struct utmp), 1, fp) == 1)
		{
			if(ut_entry.ut_type != USER_PROCESS)
				continue;
				// string entries are not 0 terminated if too long...
				// copy user name to make sure it is 0 terminated
			if(kill (ut_entry.ut_pid, 0) < 0 && errno == ESRCH) continue;
			//fprintf(userLog, "Exit status %d, Termination statuus %d\n",ut_entry.ut_exit.e_exit, ut_entry.ut_exit.e_termination );
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
		sleep (1);
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
		sleep (1);
	}
}

int CargaTotal(int a,int b,int c) {
	return (a+b+c);
}

int UsoTotal(int a,int b, int c, int d) {
	return (CargaTotal(a,b,c)+d);
}

double PorcentajeUsoCPU(int ct0,int ct1,int ut0,int ut1) {
	return 100 * (ct1 - ct0) / (ut1 - ut0);
}

int getCPUT(int *uso, int *carga) {
	FILE *archivo;

	char caracter;
	char caracteres[100];
	int posicion;
	int contador;

	int campos[4];
	int lugar;
	int final;

	archivo = fopen("/proc/stat","r");		//r: Solo lectura

	if(archivo == NULL) {		//Si no se pudo leer el archivo stat retornamos 0
		return 0;
	}

	/* Recorremos la primera linea del archivo y obtener los caracteres hasta el sexto espacio,
	luego los guardamos en un array */
	posicion = 0;
	contador = 0;
	while(contador < 6) {

		caracter = fgetc(archivo);

		if(caracter == 32) {		//Codigo asci de espacio = 32
			contador++;
		}

		caracteres[posicion] = caracter;
		posicion++;

	}

	/* Variables para calcular las cantidades*/
	lugar = 0;
	final = 0;

	/* calcular valor de c1,c2,c3,c4 */

	campos[0] = 0;
	campos[1] = 0;
	campos[2] = 0;
	campos[3] = 0;

	/* Comenzamos desde la posicion 5, ya que las primeras letras son "CPU  ", luego partimos cada vez que encontremos un espacio */
	contador = posicion;
	posicion = 5;

	while(posicion < contador) {

		if(caracteres[posicion] == 32) {		//Si es un espacio cambiamos a la siguiente posicion de campos

			if(final == 4) {		//Si tenemos las 4 posiciones del array nos salimos
				break;
			}

			posicion++;
			lugar++;
			final++;
			continue;
		}

		campos[lugar] = campos[lugar]*10 + (caracteres[posicion] - 48);		
		/* Como caracteres[posicion] devuelve codigo ascii del numero le restamos 48 (Codigo del 0)
		y asi obtenemos el valor real el numero */
		posicion++;
	}

	/* calculo de carga total y uso total
	Valores por referencia */
	*carga = CargaTotal(campos[0],campos[1],campos[2]);
	*uso = UsoTotal(campos[0],campos[1],campos[2],campos[3]);

	fclose(archivo);
	return 1;		//Si todo salio bien retornamos 1
}

double getCPUUsage() {

	int usoT0, cargaT0, usoT1, cargaT1;
	double porUso;

	/* Obtenemos el uso y la carga en el tiempo 0 */
	if(getCPUT(&usoT0, &cargaT0) == 0) {
		return -1.0;		//Si retorna 0 no se pudo leer el archivo stat y retornamos un numero negativo
	}

	sleep(1);

	/* Obtenemos el uso y la carga despues de un segundo */
	if(getCPUT(&usoT1, &cargaT1) == 0) {
		return -1.0;
	}

	porUso = PorcentajeUsoCPU(cargaT0,cargaT1,usoT0,usoT1);
	return porUso;
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
		fprintf(cpuUsage,"CPU -> %f\n",getCPUUsage());
		fclose(cpuUsage);
		sleep (1);
	}
}