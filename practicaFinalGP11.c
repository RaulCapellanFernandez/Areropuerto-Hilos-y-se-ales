/*
Grupo 11
David Alvarez Reñones
Raul Capellan Fernandez
Marco Garcia Gonzalez
Pedro Jose Pascual Ruano
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>


//variables globales
int numeroUsuarios;
int numeroPuestos;
int nombreClientes;
int aeropuertoActivo=1;
int genteEnCola=0;
int usuariosFacturados = 0;
int enDescanso = 0;


struct usuario{
	int idUsuario;
	int facturado;
	int atendido;
	int nuevo;
	int vip;
	int facturador;
	int embarque;
	pthread_t hiloUsuario;
};

struct usuario * usuarios;


struct facturador{
	int idFacturador;
	int numeroAtendidos;
	int activo;
	int vip;
	int posicionCliente;
	pthread_t hiloFacturador;
};

struct facturador * facturadores;

pthread_t seguridad;

//declaracion de funciones
int asignacionTrabajadoresPuestos(int argc,char**argv);
void iniciaSenales();
void iniciaSemaforos();
void writeLogMessage(char *id, char *msg);
void nuevoUsuario(int s);
void *accionesUsuario(void * n);
void *accionesFacturador(void * n);
void *accionesAgenteSeguridad(void * n);
void finaliza(int senal);
void sumarUsuarios(int senal);
void sumarFacturadores(int senal);
int calculoAleatorio(int max, int min);
void mataHilos(int ubicacion, struct usuario *cola);


//mutex
pthread_mutex_t semEscribe;
pthread_mutex_t semCondicion;
pthread_mutex_t semLlegaAeropuerto;
pthread_mutex_t semFacturacion;
pthread_mutex_t semSumUsu;
pthread_mutex_t semSumFac;
pthread_mutex_t semAccesoControl;
pthread_mutex_t semSeguridad;
pthread_mutex_t semControl;
pthread_mutex_t semDescanso;
pthread_cond_t condicion;


//Fichero donde se van a ubicar los logs
FILE * fichero;
const char * registro = "registrosTiempos.log";


int main(int argc, char *argv[]){
    int i;

    //facilitar el pid
	printf("************ Mi pid es %d\n", getpid());
	
	//Opcional 1 asignacion estatica de recursos
	asignacionTrabajadoresPuestos(argc,argv);

	//Se inicializa el archivo si o existe, si no lo sobreescribe
	fichero=fopen(registro,"w");
	fclose(fichero);
	writeLogMessage("","Bienvenido al aeropuerto Carles Puigdemont\n");
	
	//Reserva memoria para los usuarios y los fqacturadores
	usuarios = (struct usuario*)malloc(sizeof(struct usuario)*numeroUsuarios);
	facturadores = (struct facturador*)malloc(sizeof(struct facturador)*numeroPuestos);
	genteEnCola=0;
	
	//Crea los facturadores
	for(i=0; i<numeroPuestos; i++){
		facturadores[i].idFacturador = i+1;
		facturadores[i].numeroAtendidos=0;
		facturadores[i].activo = 1;//los facturadores empiezan activos
		facturadores[i].vip = (i+1)%2;
		facturadores[i].posicionCliente=-1;
		pthread_create(&facturadores[i].hiloFacturador, NULL, accionesFacturador, (void*)&facturadores[i].idFacturador);
	}
	
	//Crea los usuarios
	for(i=0; i<numeroUsuarios; i++){
		usuarios[i].idUsuario=0;
		usuarios[i].facturado=0;		
		usuarios[i].atendido=0;
		usuarios[i].facturador=0;
		usuarios[i].embarque=0;
		usuarios[i].vip=0;
	}
	

	pthread_create(&seguridad, NULL, accionesAgenteSeguridad, (void*)&usuarios[i].idUsuario);
	
	//Inicializa las senales que se van a usar
	iniciaSenales();
	nombreClientes = 1;
	
	while(aeropuertoActivo==1 || genteEnCola>0){
		sleep(1);
	}
}

//Se tienen que introducir los usuarios y facturadores a la vez como argumentos
int asignacionTrabajadoresPuestos(int argc,char**argv){
	if(argc == 1){
		numeroUsuarios = 10;
		numeroPuestos = 2;

	}else if(argc == 2){
		numeroUsuarios = atoi(argv[1]);
		numeroPuestos = 2;

	}else if(argc == 3){
		numeroUsuarios = atoi(argv[1]);
		numeroPuestos = atoi(argv[2]);

		if(numeroUsuarios<1 || numeroPuestos <1){
			printf("Debe haber por lo menos un usuario y un puesto\n");
			exit(0);
		}
	}else{
		printf("Argumentos incorrectos\n");
		exit(0);
	}
	printf("Hay %d usuarios.\n", numeroUsuarios);
	printf("Hay %d puestos de facturacion.\n", numeroPuestos);
}

void iniciaSenales(){

	if(signal(SIGUSR1, nuevoUsuario)==SIG_ERR){
		exit(-1);
	}

	if(signal(SIGUSR2, nuevoUsuario)==SIG_ERR){
		exit(-1);
	}
	
	if(signal(SIGINT,finaliza)==SIG_ERR){
		exit(-1);
	}

	if(signal(SIGPIPE,sumarUsuarios)==SIG_ERR){
		exit(-1);
	}

	if(signal(SIGALRM,sumarFacturadores)==SIG_ERR){
		exit(-1);
	}

}

void iniciaSemaforos(){
	
	if(pthread_mutex_init(&semEscribe, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semCondicion, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semLlegaAeropuerto, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semFacturacion, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semSeguridad, NULL)==-1){
		exit(-1);
	}
	if(pthread_cond_init(&condicion, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semSumUsu, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semSumFac, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semAccesoControl, NULL)==-1){
		exit(-1);
	}
	if(pthread_mutex_init(&semControl,NULL)==-1){
		exit(-1);
	}
	
}

void nuevoUsuario(int s){

	if(signal(SIGUSR1, nuevoUsuario)==SIG_ERR){
		exit(-1);
	}

	if(signal(SIGUSR2, nuevoUsuario)==SIG_ERR){
		exit(-1);
	}
	
	int i;
	if(aeropuertoActivo==1){
		pthread_mutex_lock(&semLlegaAeropuerto);
			if(genteEnCola<numeroUsuarios){
				for(i=0; i<numeroUsuarios; i++){
				char id[50];
				char msg[200];
					if(usuarios[i].idUsuario == 0){
						pthread_mutex_lock(&semEscribe);
							if(s==SIGUSR1){
								sprintf(msg, "Entra el usuario del monton llamado usuario_%d", nombreClientes);
								usuarios[i].vip=0;
							}
							else{
								sprintf(msg, "Entra el usuario VIP llamado usuario_%d", nombreClientes);
								usuarios[i].vip=1;
							}
							writeLogMessage("Portero", msg);
							usuarios[i].idUsuario=nombreClientes;
						pthread_mutex_unlock(&semEscribe);
						nombreClientes++;//Para que la numeracion del usuario sea correcta
						genteEnCola++;//Para que la posicion en la cola sea correcta
						//Aniade el usuario a la cola usuarios
						pthread_create(&usuarios[i].hiloUsuario, NULL, accionesUsuario, (void*)&usuarios[i].idUsuario);
						i=numeroUsuarios;
					}
				}
			}else{
				//Entra cuando la cola de usuarios está llena
				printf("********Aqui no cabemos más********\n");
			}
		pthread_mutex_unlock(&semLlegaAeropuerto);//Posible fallo?
	}
}

void *accionesUsuario(void * n){
	int identificador = *(int *)n;
	int pos=0; //Posicion en la cola
	int cuentaTiempo=0;
	char id[50];
	char id2[50];
	char msg[200];
	int i;
	int clienteNormal;//Varaible para conocer si es normal o VIP
	int nEsperar=calculoAleatorio(5,1);//Para cuando el usuario esta cansado de esperar
	
	for(i=0; i<numeroUsuarios; i++){
		if(usuarios[i].idUsuario == identificador){
			pos = i;                  
		}
	}

	sprintf(id, "Usuario_%d", identificador);
	sprintf(msg, "¿Quién da la veeeeez?");
	pthread_mutex_lock(&semEscribe);
		writeLogMessage(id, msg);
	pthread_mutex_unlock(&semEscribe);

	//Si el numero aleatorio ha salido 1 es que el usuario se ha cansado de esperar.
	if(nEsperar==1){
		pthread_mutex_lock(&semEscribe);
			writeLogMessage(id, "Hasta los coj... de esperar. Chao pescao.");
		pthread_mutex_unlock(&semEscribe);
		mataHilos(pos, usuarios);	
	}

	
	do{
	
		//miro cada 3 segundos si tengo que ir al baño, y si la respuesta es afirmativa, 30% de las veces, mato al hilo
		if(cuentaTiempo==3){ 
			if(calculoAleatorio(10, 1)==3 && usuarios[pos].atendido==0){//no sea que me toque el turno y vaya al baÃ±o
				//escribo que tengo que ir al banyo
				pthread_mutex_lock(&semEscribe);
					writeLogMessage(id, "Pasoooooooooo, un baño, mi reino por un baño, no llego, no llego.");
				pthread_mutex_unlock(&semEscribe);
				mataHilos(pos, usuarios);
			}
			cuentaTiempo=0;
		}
		sleep(1);
		cuentaTiempo++;
	}while(usuarios[pos].atendido==0);
	
	sprintf(id2, "facturador_%d a usuario_%d", usuarios[pos].facturador,usuarios[pos].idUsuario);
	//cuando me dan la orden de entrar en el puesto de facturacion escribo
	pthread_mutex_lock(&semEscribe);
		writeLogMessage(id, "Y se queja la gente de las listas de espera de la sanidad.");
	pthread_mutex_unlock(&semEscribe);
	
	//espero el tiempo necesario
	int causaPunible = calculoAleatorio(10, 1);
	int tiempoEspera;
	int usuarioTieneQuePasarElControl;
	if(causaPunible<9){//paso como un campeon
		tiempoEspera = calculoAleatorio(4, 1);
		sleep(tiempoEspera);
		usuarioTieneQuePasarElControl = 1;
		//escribo que paso bien
		pthread_mutex_lock(&semEscribe);
			char msg2[70];
			sprintf(msg2, "Pasas limpio al próximo curso. Ha tardado %d segundos.",tiempoEspera);
			writeLogMessage(id2, msg2);
			usuariosFacturados++;
		pthread_mutex_unlock(&semEscribe);
	}else if(causaPunible==9){//me paso con el equipaje
		tiempoEspera = calculoAleatorio(6, 2);//Tiempo que tiene que esperar a que le revisen la maleta
		sleep(tiempoEspera);
		usuarioTieneQuePasarElControl = 1;
		pthread_mutex_lock(&semEscribe);
			char msg3[150];
			sprintf(msg3, "¿Realmente necesita 10 maletas para dos días?, recargo, por tonto. Has tardado %d segundos.",tiempoEspera);
			writeLogMessage(id2, msg3);
			usuariosFacturados ++;
		pthread_mutex_unlock(&semEscribe);

	}else{//no paso, a la trena
		tiempoEspera = calculoAleatorio(10, 6);
		sleep(tiempoEspera);
		usuarioTieneQuePasarElControl = 0;
		pthread_mutex_lock(&semEscribe);
			char msg4[150];
			sprintf(msg4, "Motivo del viaje... terrorismo internacional, no puedo dejarle pasar, mejor suerte la próxima. Has tardado %d segundos.",tiempoEspera);
			writeLogMessage(id2, msg4);
		pthread_mutex_unlock(&semEscribe);
		mataHilos(pos,usuarios);//Mata al hilo
		
	}

	
	
	//escribo que paso al puesto de seguridad
	pthread_mutex_lock(&semEscribe);
		writeLogMessage(id, "Agente, agente, despierte leche que me tiene que controlar");
	pthread_mutex_unlock(&semEscribe);
	
	
	
	
	pthread_mutex_lock(&semAccesoControl);
		pthread_mutex_lock(&semCondicion);
			usuarios[pos].facturado=1;
			sleep(1);
			pthread_cond_signal(&condicion);//echar al cliente
	
			
		pthread_mutex_unlock(&semCondicion);
		while(usuarios[pos].embarque==0){
			sleep(1);
		}
	pthread_mutex_unlock(&semAccesoControl);
	//espero en seguridad


	
	pthread_mutex_lock(&semEscribe);
		writeLogMessage(id, "Por finnnn, después de dos días y medio, ya os vale.");
	pthread_mutex_unlock(&semEscribe);

	if (usuarioTieneQuePasarElControl==1){
		pthread_mutex_lock(&semEscribe);
			writeLogMessage(id, "Muchacho te ha tocado pasar el control");
		pthread_mutex_unlock(&semEscribe);
	}
	
	

	//Ha pasado el control de seguridad y embarca en el avion
	if(usuarios[pos].embarque==1){
		pthread_mutex_lock(&semEscribe);
			writeLogMessage(id, "Ha ido dirección Bruselas.\n");
		pthread_mutex_unlock(&semEscribe);
	}

	mataHilos(pos,usuarios);//Mata al hilo
}

void *accionesFacturador(void * n){
	int identificador = *(int*)n;
	int i;
	int pos;
	struct usuario *cola;
	char id[50];
	char msg[200];
	int cafe=0;
	int ocupado=0;


	for(i=0; i<numeroPuestos; i++){
		if(facturadores[i].idFacturador == identificador) pos = i;
	}
		
	pthread_mutex_lock(&semEscribe);
		sprintf(id, "facturador %d", identificador);
		writeLogMessage(id, "Puesto de facturacion abierto\n");
	pthread_mutex_unlock(&semEscribe);
	
	do{
		pthread_mutex_lock(&semFacturacion);
			facturadores[pos].posicionCliente = -1;
		pthread_mutex_unlock(&semFacturacion);
		while(facturadores[pos].posicionCliente==-1){
		int pescado=10000;
		sleep(1);
			pthread_mutex_lock(&semFacturacion);//No entren dos usurios por el mismo puesto de  facturacion
				if(facturadores[pos].vip==1){
					for(i=0; i<numeroUsuarios; i++){
						if(usuarios[i].vip==1 && usuarios[i].idUsuario!=0 && usuarios[i].atendido==0 && usuarios[i].idUsuario<pescado){
							facturadores[pos].posicionCliente = i;
							usuarios[i].facturador = identificador;
							i=numeroUsuarios;
							pescado = usuarios[i].idUsuario;
							
						}
					}
				}

				if(facturadores[pos].posicionCliente < 0){
					for(i=0; i<numeroUsuarios; i++){
						if(usuarios[i].vip==0 && usuarios[i].idUsuario!=0 && usuarios[i].atendido==0 && usuarios[i].idUsuario<pescado){
							facturadores[pos].posicionCliente = i;
							usuarios[i].facturador = identificador;
							i=numeroUsuarios;
							pescado = usuarios[i].idUsuario;
						}
					}
				}
				
				if(facturadores[pos].posicionCliente>0){//LLama al siguiente usuario de la cola
					
					sprintf(msg, "He pescado al cliente llamado usuario_%d", usuarios[facturadores[pos].posicionCliente].idUsuario);
					pthread_mutex_lock(&semEscribe);
						writeLogMessage(id, msg);
					pthread_mutex_unlock(&semEscribe);
				}
				sleep(1);
			pthread_mutex_unlock(&semFacturacion);
				
			}
		pthread_mutex_lock(&semDescanso);
			if (cafe >= 5){//El facturador se va a toamar un cafe cuando ha atendido a 5 usuarios
				if(enDescanso ==0 ){
					enDescanso =1;
					pthread_mutex_lock(&semEscribe);
						writeLogMessage(id, "Se marcha a tomar un cafetito y a charlar con las compañeras de trabajo el pillin.");
					pthread_mutex_unlock(&semEscribe);

					sleep(10);//Tiempo de espera

					pthread_mutex_lock(&semEscribe);
						writeLogMessage(id, "Vuelta a la mierd... de trabajo cruck.");
					pthread_mutex_unlock(&semEscribe);
					cafe = 0;
					enDescanso =0;
				}
			}	
		pthread_mutex_unlock(&semDescanso);	

		if(facturadores[pos].posicionCliente>=0) usuarios[facturadores[pos].posicionCliente].atendido=1;		
					
		cafe++;//Un usuario atendido
			
		while(usuarios[facturadores[pos].posicionCliente].facturado==0){
			sleep(0.5);
		}
	}while(aeropuertoActivo || genteEnCola>0);	
}

void *accionesAgenteSeguridad(void * n){
	int identificador = *(int*)n;
	int pos;
	int usuarioNormal;
	int i;
	char id[50];
	int cafeSeg = 0;
	
	
	while(aeropuertoActivo==1 || genteEnCola>0){
		pthread_mutex_lock(&semCondicion);
			pthread_cond_wait(&condicion, &semCondicion);
		pthread_mutex_unlock(&semCondicion);
		for(i=0; i<numeroUsuarios; i++){
			if(usuarios[i].facturado==1){
				identificador = usuarios[i].idUsuario;
				pos = i;
				i=numeroUsuarios;
			}			
		}
		
		//LLega el usuario al control de seguridad
		sprintf(id, "Agente Seguridad");
		pthread_mutex_lock(&semEscribe);
			writeLogMessage(id, "Ha llegado al control de seguridad\n");
		pthread_mutex_unlock(&semEscribe);

		int controlSeguridad = calculoAleatorio(10,1); //Variable para decidir si tiene que pasar o no por el control de seguridad (1-3 No pasa control// 4-5 Si pasa)	
		int tiempoEspera;

		if(controlSeguridad >= 4){

			tiempoEspera = calculoAleatorio(3, 2);//Tiempo que tiene que esperar en el control
			sleep(tiempoEspera);
			pthread_mutex_lock(&semEscribe);
				writeLogMessage(id, "Ha superado el control de seguridad sin ser inspeccionado\n");
			pthread_mutex_unlock(&semEscribe);

			pthread_mutex_lock(&semSeguridad);
				if(usuarioNormal==1){//Para que el usuario pueda embarcar en el avion
					usuarios[pos].embarque=1;
				}else{
					usuarios[pos].embarque=1;
				}
			pthread_mutex_unlock(&semSeguridad);

		}else{

			tiempoEspera = calculoAleatorio(15,10);//Tiempo que va a tener que esperar en la inspeccion
			pthread_mutex_lock(&semEscribe);
				writeLogMessage(id, "Está siendo inspeccionado\n");
			pthread_mutex_unlock(&semEscribe);

			sleep(tiempoEspera);//Espera el tiempo de arriba

			pthread_mutex_lock(&semEscribe);
				writeLogMessage(id, "Acaba de salir de la inspección\n");
			pthread_mutex_unlock(&semEscribe);

			pthread_mutex_lock(&semSeguridad); //Embarca despues de esperar en la inspeccion
				usuarios[pos].embarque=1;
			pthread_mutex_unlock(&semSeguridad);
		}
		pthread_mutex_lock(&semDescanso);
			if (cafeSeg >= 5){//El de seguridad se va a toamar un cafe cuando ha atendido a 5 usuarios
				if(enDescanso == 0){
					enDescanso =1;
					pthread_mutex_lock(&semEscribe);
						writeLogMessage(id, "Agente de seguridad se va a tomar un cafe");
					pthread_mutex_unlock(&semEscribe);

					sleep(2);//Tiempo de espera

					pthread_mutex_lock(&semEscribe);
						writeLogMessage(id, "Vuelta a la mierd... de trabajo cruck.");
					pthread_mutex_unlock(&semEscribe);
					cafeSeg = 0;
					enDescanso =0;
				}
			}
		pthread_mutex_unlock(&semDescanso);		
		cafeSeg++;	
		
	}
}


void finaliza(int senal){
	char id[50];
	
	aeropuertoActivo = 0;

	if(signal(SIGINT,finaliza)==SIG_ERR){
		exit(-1);
	}
	
	//Esperamos que todos los usuarios acaben
	while(genteEnCola>0){
		sleep(1);
	}
	
	pthread_mutex_unlock(&semFacturacion);

	pthread_mutex_lock(&semEscribe);
		sprintf(id, "%d", usuariosFacturados);
		writeLogMessage(id, "Numero de usuarios que han sido facturados.");
		writeLogMessage("",".......Termina la ejecucion del programa.......");
	pthread_mutex_unlock(&semEscribe);

	sleep(1);
	
	exit(0);//Cierra el programa
}

//Señal 13 Opcional 2 Asignacion Dinamica de Usuarios
void sumarUsuarios(int senal){
	char id[50];

	if(signal(SIGPIPE,sumarUsuarios)==SIG_ERR){
		exit(-1);
	}
	
	pthread_mutex_lock(&semSumUsu);
		numeroUsuarios++;
	pthread_mutex_unlock(&semSumUsu);

	pthread_mutex_lock(&semEscribe);
		writeLogMessage("","Se ha añadido un espacio más de usuarios!");
	pthread_mutex_unlock(&semEscribe);
}

//Señal 14 Opcional 3 Asignacion Dinamica de facturadores
void sumarFacturadores(int senal){

	if(signal(SIGALRM,sumarFacturadores)==SIG_ERR){
		exit(-1);
	}
	
	pthread_mutex_lock(&semSumFac);
		//Crea un nuevo facturador
		facturadores[numeroPuestos].idFacturador = numeroPuestos+1;
		facturadores[numeroPuestos].numeroAtendidos=0;
		facturadores[numeroPuestos].activo = 1;//los facturadores empiezan activos
		facturadores[numeroPuestos].vip = (numeroPuestos)%2;
		facturadores[numeroPuestos].posicionCliente=-1;
		pthread_create(&facturadores[numeroPuestos].hiloFacturador, NULL, accionesFacturador, (void*)&facturadores[numeroPuestos].idFacturador);
		numeroPuestos++;
	pthread_mutex_unlock(&semSumFac);

	pthread_mutex_lock(&semEscribe);
		writeLogMessage("","Se ha añadido un facturador más!");
	pthread_mutex_unlock(&semEscribe);
}


void mataHilos(int ubicacion, struct usuario *cola){
	genteEnCola--;
	cola[ubicacion].idUsuario = 0;
	cola[ubicacion].atendido = 0;
	cola[ubicacion].facturado = 0;
	cola[ubicacion].embarque = 0;
	//cola[ubicacion].vip = 0;
	pthread_exit(&cola[ubicacion].hiloUsuario);
}

void writeLogMessage(char *id, char *msg){
	//  Calculamos  la hora  actual
	time_t now =time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow,19, "%d/%m/%y %H:%M:%S", tlocal);
	//  Escribimos  en el log
	fichero = fopen(registro, "a");
	fprintf(fichero,"[%s] %s: %s\n", stnow, id,msg);
	fclose(fichero);
	printf("[%s] %s: %s\n", stnow, id,msg);
}

int calculoAleatorio(int max, int min){
	
	srand(time(NULL));
	int numeroAleatorio = rand()%((max+1)-min)+min;
	return numeroAleatorio;
}
