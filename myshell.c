#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <wait.h>
#include <fcntl.h>

#include "parser.h"

// Compilar: gcc -Wall -Wextra myshell.c libparser.a -o myshell -static

void prompt(); // 1
void oneCommandProcess(); // 2
void twoCommandProcess(); // 3
void moreTwoCommandProcess(); // 4
void backgroundCommand(); // 5
void redirectionProcess(); // 6


// 1
void prompt(){
    char *p = getenv("USER");
    printf("%s_msh >",p);
    //printf(msh > );
}


// 2
void oneCommandProcess(char **argv){

    pid_t pid;
    pid = fork();

    if (pid==0){
        //printf("Hijo\n");

        if (execvp(argv[0],argv)<0){ //Ejecuta el comando.
            //Error
            printf("No se puede ejecutar el comando.\n");
        }
        exit(0);

    } else{
        //printf("Padre\n");
        wait(NULL); // Espera al hijo, evita hijo zombie.
        //return;
    }
}


// 3
void twoCommandProcess(pid, buf){
    int pipe_des[2];
    FILE *fd;

    if (pid==0){  // hijo
        close(pipe_des[1]);  // hijo solo recibe, cerramos escribir
        fd = fdopen(pipe_des[0], "r");  // tenemos FILE* con el descriptor del pipe
        fgets(buf, 1024, fd);
        fclose(fd);
        printf("Hijo -> Recibido lo siguiente: \"%s\"\n", buf);

        //Ejemplo con ls:
        /*char *arg_list[] = {"ls", "-l", NULL};


        if (execvp("ls", arg_list)<0){
            //Error
            printf("No se puede ejecutar el comando.\n");
        }
        //exit(0);
        */
    } else{  // padre
        close(pipe_des[0]);  // solo envia, no lee
        fd = fdopen(pipe_des[1], "w");
        fprintf(execvp())
        wait(NULL); //
        return;
    }
    exit(0);
}


// 4
void moreTwoCommandProcess(pid, pidHijos, ncommands){
    pidHijos = malloc(ncommands * sizeof (int));

    if (pid==0){  // hijo
        close()
        printf("Hijo\n");
    } else{
        printf("Padre\n");
    }

}

// 5
void  backgroundCommand(pid){

    printf("Mostramos pid de proceso en background es: %d \n", pid);
}


// 6
void redirectionProcess(tline *line){

    if (line->redirect_input){
        // Hay redirección de entrada.

        // Ejemplo: sort < .txt. Creamos descriptor de fichero f. Lee lo que hay en .txt y
        // dup2 copia f en 0, que sirve como stdin (es 0) de sort.

        int f;
        f = open(line->redirect_input, O_RDONLY); //Creamos descriptor de archivo solo de lectura.

        if (dup2(f,0)<0){
            //Error
            printf("No se puede ejecutar el comando.\n");
            close(f);
            exit(-1);
        }
        printf("LLego hasta aqui");
        close(f);
        
    }
    else if (line->redirect_output){
        // Hay redirección de salida.

        // Ejemplo: ls > .txt. Creamos descriptor de fichero f. Puede escribir, crear y truncar lo que hay en .txt
        // dup2 copia f en 1, que sirve como stdout (es 1) de ls. Ejecutamos el comando ya dentro de f, ya que f es el stdout.

        int f;
        f = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0600); //Creamos descriptor de archivo write,create,trunc, con permisos 0600.

        dup2(f,1); //stdout. Imprime por pantalla.
        execv(line->commands[0].filename, line->commands[0].argv);
        close(f);
        /*
        if (dup2(f,1)<0){
            //Error
            printf("No se puede ejecutar el comando.\n");
        }
        */

    }
    else if (line->redirect_error){
        // Hay redirección de error.
        //int f;
        //dup2(f,2); //stderr

    }

}


/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////

int main(void) {

	char buf[1024];
	tline * line;

    pid_t pid;
    pid = fork();
    int *pidHijos;

    int **pipes;  // matriz de pipes

	//int i,j;

    //signal(SIGINT,SIG_IGN); // En teoría ignoramos señales Ctrl + C y Ctrl + Z,
    //signal(SIGQUIT,SIG_IGN);// pero solo funciona con la primera! (En sitios ponen SIGQUIT)

    prompt(); // Para que aparezca al ejecutar

	while (fgets(buf, 1024, stdin)) { // DUDA
		line = tokenize(buf); //Cogemos una línea por teclado.

        if (line==NULL) { // Si está vacía continuamos hasta mostrar prompt.
            continue;
		}

        // Si hay redirecciones de cualquier tipo:
        if (line->redirect_input || line->redirect_output || line->redirect_error ) {
			//printf("redirección de entrada: %s\n", line->redirect_input);
            redirectionProcess(line);
		}

        // Si se ejecuta en background:
		if (line->background) {
			printf("Comando a ejecutarse en background\n");
            backgroundCommand(pid);

        }

        // Diferente número de comandos introducidos:
        if (line->ncommands == 1){ // Añadir cd.
            //printf("Un solo comando introducido\n");
            oneCommandProcess(line->commands->argv); // Procesamos 1 solo comando en la función con x mandatos.
        }
        else if(line->ncommands == 2){
            printf("2 argumentos: Implemetación con 1 pipe\n");
            twoCommandProcess(pid, buf);
        }
        else if(line->ncommands >= 2){
            printf("2 o mas argumentos: Implementación con 1 o mas pipes\n");
            moreTwoCommandProcess(pid, pidHijos, line->ncommands);
        }


        /*
		for (i=0; i<line->ncommands; i++) { //Recorremos array de comandos.
            //execvp(line->commands->argv[0],line->commands->argv); // Me ejecuta el comando, pero se sale!
            // Con execvp, si sigue hacia abajo es que se ha producido un error...
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j=0; j<line->commands[i].argc; j++) { // Recorremos array de argumentos de comando.
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
		}
        */


        printf("Llego hasta aquiiii");
        prompt(); // Para que aparezca cada salto
	}

	return 0;
}


/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////
