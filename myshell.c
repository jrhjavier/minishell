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
    printf("ONE COMMAND:\n");

        //printf("Hijo\n");
    if (execvp(argv[0],argv)<0){ //Ejecuta el comando.
        //Error
        printf("No se puede ejecutar el comando.\n");
        exit(-1);
    }
}

/*
// 3
void twoCommandProcess(){


    pid_t pid;
    pid = fork();

    if (pid==0){
        printf("Hijo\n");

        //Ejemplo con ls:
        char *arg_list[] = {"ls", "-l", NULL};


        if (execvp("ls", arg_list)<0){
            //Error
            printf("No se puede ejecutar el comando.\n");
        }
        //exit(0);

    } else{
        printf("Padre\n");
        wait(NULL); //
        return;
    }
    exit(0);
}
*/

// 4
void moreTwoCommandProcess(int pid){

    if (pid==0){
        printf("Hijo\n");
    } else{
        printf("Padre\n");
    }

}

// 5
void  backgroundCommand(int pid){

    printf("Mostramos pid de proceso en background es: %d \n", pid);
}


// 6 --
void redirectionProcess(tline *line, pid_t pid){
    printf("REDIRECTION:\n");

    pid_t a = pid;
    printf("%d",a);

    if (line->redirect_output) {

        // Ejemplo: sort < .txt. Creamos descriptor de fichero f. Lee lo que hay en .txt y
        // dup2 copia f en 0, que sirve como stdin (es 0) de sort.

        int f;
        f = open(line->redirect_output, O_RDONLY); //Creamos descriptor de archivo solo de lectura.

        if (f == -1) {
            printf("ERROR");
        } else {
            dup2(f, 1);
        }
        execv(line->commands[0].filename, line->commands[0].argv);
    }

    /*
    else if (line->redirect_output){
        // Hay redirección de salida.

        // Ejemplo: ls > .txt. Creamos descriptor de fichero f. Puede escribir, crear y truncar lo que hay en .txt
        // dup2 copia f en 1, que sirve como stdout (es 1) de ls. Ejecutamos el comando ya dentro de f, ya que f es el stdout.

        int f;
        f = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0600); //Creamos descriptor de archivo write,create,trunc, con permisos 0600.

        dup2(f,1); //stdout. Imprime por pantalla.
        execv(line->commands[0].filename, line->commands[0].argv);
        close(f);
        *
        if (dup2(f,1)<0){
            //Error
            printf("No se puede ejecutar el comando.\n");
        }
        *

    }
    else if (line->redirect_error){
        // Hay redirección de error.
        //int f;
        //dup2(f,2); //stderr

    }
    */

}


/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////

int main(void) {

	char buf[1024];
	tline * line;
	pid_t pid;
    int status;
	//int i,j;

    //signal(SIGINT,SIG_IGN); // En teoría ignoramos señales Ctrl + C y Ctrl + Z,
    //signal(SIGQUIT,SIG_IGN);// pero solo funciona con la primera! (En sitios ponen SIGQUIT)


    prompt();

    while (fgets(buf, 1024, stdin)) { // DUDA

        pid = fork();

        if (pid == 0){

            //prompt();
            line = tokenize(buf); //Cogemos una línea por teclado.

            if (line==NULL) { // Si está vacía continuamos hasta mostrar prompt.
                continue;
            }
                // Si hay redirecciones de cualquier tipo:
            else if (line->redirect_input || line->redirect_output || line->redirect_error ) {
                //printf("redirección de entrada: %s\n", line->redirect_input);
                redirectionProcess(line, pid);
            }
                // Si se ejecuta en background:
            else if (line->background) {
                printf("Comando a ejecutarse en background\n");
                backgroundCommand(pid);
            }
                // Diferente número de comandos introducidos:
            else if (line->ncommands == 1){ // Añadir cd.
                //printf("Un solo comando introducido\n");
                oneCommandProcess(line->commands->argv); // Procesamos 1 solo comando en la función con x mandatos.
            }
                //else if(line->ncommands == 2){
                //    printf("2 argumentos: Implemetación con 1 pipe\n");
                //    twoCommandProcess();
                //}
            else if(line->ncommands >= 2){
                printf("2 o mas argumentos: Implementación con 1 o mas pipes\n");
                moreTwoCommandProcess(pid);
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
            prompt(); // Para que aparezca cada salto
        } else{
            wait(&status);
            prompt();
            //exit(0);
        }

    }


	return 0;
}


/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////
