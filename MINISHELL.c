#include <stdio.h>
#include "parser.h"
#include <sys/wait.h>
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h>
#include <wait.h> 
#include <string.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>

   


//PARA COMPILAR: gcc -Wall -Wshadow -o minishell main.c libparser_64.a -static

//CABECERAS DE FUNCIONES A USAR
int comandoValido (char * comando);
void ejecComando(tline * line);
void ejecPipes(tline * line);
void ejec_cd ();
mode_t mostrarMask(void);
int cambiar_mascara(char * mascara);
void umasc_simbol(mode_t mode, char * buf);
void ejec_umask();
void enter(int sig);
int redir ( char * entrada , char * salida , char * error);


//VARIABLES GLOBALES
#define CD "cd" // Constante para hacer cd 
#define UMASK "umask"
#define EXIT "exit"
tline * line; 
int status;
pid_t pid;


int comandoValido (char * comando){ // Ver si los comandos son validos
	if(comando == NULL ){
		return 1;
	} else {
		return 0;
	}
}

void ejecComando(tline *line1){
			
				pid = fork();
				
				if (pid < 0) {  // Error fork
					fprintf(stderr, "Error en el fork().\n%s\n", strerror(errno));
					exit(1);
				}
				else if (pid == 0) { // Hace hijo 
					signal(SIGINT, SIG_IGN);
					if(comandoValido(line->commands[0].filename)==0){ // Mirar si es un comando Válido
						execvp(line->commands[0].filename, line->commands[0].argv ); // Si pasa del exec , es que hay error
						
						// devuelve - 1 y salta el error 
						//fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
					     exit(1); //lo quito para seguir con el prompt
						
					} else {
						fprintf(stderr, "%s : No se encuentra el mandato\n" , line->commands[0].argv[0]); // El comando no es válido, mostrarlo
						
					}					
				}else{ 	
					wait (&status);
					
				}

}

void ejecPipes(tline * line2){
	int i;
	int p[64][2];

			
			
			
			
			
			for(i=0;i<line->ncommands-1;i++){
				pipe(p[i]);
			}
			
			for(i=0;i<line->ncommands;i++){
				
				if(comandoValido(line->commands[i].filename)==0){ 
				
					pid=fork();				
					if(pid<0){ //Error fork
						fprintf(stderr, "Error en el fork().\n%s\n", strerror(errno));
						exit(1);
					} else if (pid==0){ 
						signal(SIGINT, SIG_IGN);
						if(i==0){ //Primer hijo
							close(p[i][0]); // se cierra el extremo que NO se usa
							dup2(p[i][1],1); // se redirige el stdout al extremo de salida del 1er argumento
							execvp(line->commands[i].argv[0], line->commands[i].argv);
							// si sigue después del exec significa que ha habido un error y que exec ha devuelto -1
							fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
							exit(1);
							
						}else if(i==(line->ncommands-1)){	// Ultimo hijo
							close(p[i-1][1]);// se cierra el extremo que NO se usa
							dup2(p[i-1][0],0); //se redirige el stdin al extremo de salida del último argumento
							execvp(line->commands[i].argv[0], line->commands[i].argv);

							fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
							exit(1);
							
						}else{	// Resto de hijos			
							close(p[i][0]);	
							close(p[i-1][1]);	
							dup2(p[i-1][0],0);	
							dup2(p[i][1],1);
							execvp(line->commands[i].argv[0], line->commands[i].argv);	

							fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
							exit(1);
							
						}

					}else{	//Padre
						if(!(i==(line->ncommands-1))){
							close(p[i][1]);
						}
					}
				}else { 
					fprintf(stderr, "%s : No se encuentra el mandato\n" , line->commands[i].argv[0]);
					
				}		
			}
			// Esperar que termine todo
			for (i = 0; i < (line->ncommands); i++) {
				wait(NULL);
			}
			
			}

void ejec_cd(){
	
	char *home; 
	int e; // para saber si la llamada a sistema ha sido bien
	
	if(line->commands[0].argv[1] != NULL){ // si hay algo a la derecha del cd
		e = chdir(line->commands[0].argv[1]); // se cambia el directorio hacia el argumento siguiente al cd
		if (e < 0){ //como es una llamada a sistema me puede dar error
			fprintf(stderr, "%s: directorio incorrecto\n ", line->commands[0].argv[1]);
			exit(1);
			}
			
			else 
				printf("Cambiando a directorio: %s\n",line->commands[0].argv[1]);
		} // si no hay argumentos, es decir, si está solo el cd
		else{
			home = getenv("HOME"); // devuelve el valor asociado a una cadena
			e = chdir(home);
			if (e < 0){//como es una llamada a sistema me puede dar error
			fprintf(stderr, "%s: directorio incorrecto\n ", home);
			}// no lleva exit pq se hace sobre el padre directamente
			
			else 
				printf("Cambiando a directorio: %s\n",home);
		} 
			}
			
mode_t mostrarMask(void)
           {
			   mode_t umask(mode_t mask);
               mode_t mask = umask(0);
               umask(mask);
               return mask;
           }
           
int cambiar_mascara(char * mascara) 
{
    mode_t nueva_masc = 0;
    char c = *mascara;
    do
    {
        if(c < '0' || c > '7')
        {	fprintf(stderr,"La expresión introducida es errónea\n");
            return 0;
        }
        nueva_masc <<= 3;
        nueva_masc += c - '0';
        if(nueva_masc > 0777)
        {	fprintf(stderr,"Número octal excede el máx permitido (0777)\n");
            return 0;
        }
        mascara += 1;
        c = *mascara;
    }
    while(c != '\0'); //hasta que no haya más caracteres
    umask(nueva_masc);
    return 1;
}
           
void umasc_simbol(mode_t mode, char * buf) {
  const char perms[] = "rwxrwxrwx";
  for (size_t i = 0; i < 9; i++) {
    buf[i] = !(mode & (1 << (8-i))) ? perms[i] : '-'; 
  }
  buf[9] = '\0';
}
           
void ejec_umask(char * arg0){
	
	char perms[10];
	
	if (line->commands[0].argc==1){ 
		printf("%04o\n", mostrarMask());
		}
	else{
		
		if (strcmp(line->commands[0].argv[1], "-S") == 0){
		umasc_simbol(mostrarMask(), perms); 
		printf("%s\n", perms);
		 }
		
		else{	
		
		cambiar_mascara(line->commands[0].argv[1]);
		}
	}
		
		}
	
						
int redir ( char * entrada , char * salida , char * error ){
	
	int aux; //auxiliar para redirigir 

	// Si es de entrada 
	//Para que te deje abrir y editar el documento:
	if(entrada != NULL ) {
		aux = open (entrada , O_CREAT | O_RDONLY); 
		if(aux == -1){
			fprintf( stderr , "%s : Error. %s\n" , entrada , strerror(errno)); // Mostrar error , -1 igual a NULL 
			return 1;
		} else { 
			dup2(aux,0); //Entrada estandar 
		}	
	}
	
	// Si es de Salida
	if(salida != NULL ) {
		aux = creat (salida ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ); 
		if(aux == -1){
			fprintf( stderr , "%s : Error. %s\n" , salida , strerror(errno)); 
			return 1;
		} else { 
			dup2(aux,1); //Salida estandar 
		}	
	}
	
	// Si es de error 
	if(error != NULL ) {//user read permission | group write permission | group read | group write | others read | others write 
		aux = creat (error ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(aux == -1){
			fprintf( stderr , "%s : Error. %s\n" , error , strerror(errno)); // Mostrar error , -1 igual a NULL 
			return 1;
		} else { 
			dup2(aux,2); //Error estandar 
		}	
	}
	
	return 0;
}

void enter(int sig){
	fprintf(stderr, "\n\033[1;32mmsh\033[0m>  ");
}	
	
int main(void) {

	char buffer[1024];
	
	// Guardar entradas estandar para posible redireccion
	int rEntrada = dup(0); 
	int rSalida = dup(1);
	int rError = dup(2);

	//     "VERDEmshBLANCO>      
	printf("\033[1;32mmsh\033[0m> ");
	//SIGINT == Ctrl+C 
	signal(SIGINT,(void (*)(int))enter); 
	
	while (fgets(buffer, 1024, stdin)) {
		
		signal(SIGINT,(void (*)(int))enter);
		line = tokenize(buffer);		

		if (line->redirect_input != NULL) {
			redir ( line->redirect_input , NULL , NULL ); 
		}
		if (line->redirect_output != NULL) {
			redir ( NULL , line->redirect_output , NULL ); 
		}
		if (line->redirect_error != NULL) {
			redir ( NULL , NULL , line->redirect_error ); 
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
			return(1);
		} 
		
		if(line->ncommands==1){
		
			signal(SIGINT , (void (*)(int))enter);
			
			
			// Comprueba si es cd , umask o exit, en caso contrario ejecuta el comando introducido
			if (strcmp(line->commands[0].argv[0], CD) == 0){
				ejec_cd();
				}
			else if (strcmp(line->commands[0].argv[0], UMASK) == 0){
				ejec_umask(line->commands[0].argv[0]);
				}
			
			else if (strcmp(line->commands[0].argv[0], EXIT) == 0){
				exit(0);
				}
			else{ 
				ejecComando(line);
			}
			
			
		}else if(!(line->ncommands == 1)){ // Si son 2 comandos o más
			ejecPipes(line);
		}
		
		// Restablecemos las redirecciones 
		
		if(line->redirect_input != NULL ){
			dup2(rEntrada , 0);	
		}
		if(line->redirect_output != NULL ){
			dup2(rSalida , 1);	
		}
		if(line->redirect_error != NULL ){
			dup2(rError , 2);	
		}
		
		
		
		printf("\033[1;32mmsh\033[0m> ");
		
		
	}
	return 0;
	
}
