#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "parser.h"

// Compilar: gcc -Wall -Wextra myshell.c libparser.a -o myshell -static

void prompt(); // 1
void oneCommandProcess(); // 2
void twoCommandProcess(); // 3
void moreTwoCommandProcess(); // 4
void backgroundCommand(); // 5
void redirectionProcess(); // 6
void mycd(); // 7


// 1
void prompt(){
    char *p = getenv("USER");
    printf("%s_msh >",p);
    //printf(msh > );
}


// 2
void oneCommandProcess(tline *line){

    if(strcmp(line->commands->argv[0],"cd") == 0){ //Comano cd
        mycd(line);
    }

    else if (line->redirect_input || line->redirect_output || line->redirect_error ) {
        //printf("redirección de entrada: %s\n", line->redirect_input);
        redirectionProcess(line);
    }

    else {

        if (execvp(line->commands->argv[0], line->commands->argv) < 0) { //Ejecuta el comando.
            // Error
            printf("%s: No se encuentra el mandato.\n", line->commands->argv[0]);
            exit(-1);
        }
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
void moreTwoCommandProcess(pid_t pid, int *pidHijos, int ncommands, int **pipes){
    pidHijos = (pid_t) malloc(ncommands * sizeof (int));  // creamos tantos pids como comandos haya
    pipes = malloc((ncommands - 1) * sizeof(int *));  // reservamos memoria para matriz de pipes

    for (int i = 0; i < ncommands - 1; ++i) {  // creamos tantos pipes como comandos - 1 haya
        pipes[i] = (int *) malloc(sizeof (int) * 2);
        if(pipe(pipes[i]) < 0){
            fprintf(stderr, "Error al crear el pipe %s\n", strerror(errno));
        }
    }

    for (int i = 0; i < ncommands; ++i) {  // un hijo por cada comando
        pid = fork();
    }
    if (pid==0){  // hijo
        close()
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
void redirectionProcess(tline *line){


    if (line->redirect_input) {

        // Hay redirección de entrada.

        // Ejemplo: sort < .txt. Creamos descriptor de fichero f. Lee lo que hay en .txt y
        // dup2 copia f en 0, que sirve como stdin (es 0) de sort.

        int f; // abrimos gile.txt como escritura!!
        int errnum; // Guardar numero de error
        f = open(line->redirect_input, O_RDONLY); //Creamos descriptor de archivo solo de lectura.

        if ( f != -1){

            dup2(f,STDIN_FILENO); // La stdin de lo que ejecutemos en adelante será lo que contiene el descriptor f.

            if (execvp(line->commands->argv[0],line->commands->argv) < 0){ //Ejecuta el comando.
                //Error
                printf("%s: No se encuentra el mandato.\n",line->commands->argv[0]);
                exit(-1);
            }

        }else{

            errnum = errno; // Guardo numero de error
            fprintf(stderr,"%s: Error: %s\n", line->redirect_input, strerror(errnum)); // Imprimo "Error: No such file or directory."

        }

    }

    if (line->redirect_output) {

        // Hay redirección de salida.

        // Ejemplo: ls > .txt. Creamos descriptor de fichero f. Puede escribir, crear y truncar lo que hay en .txt
        // dup2 copia f en 1, que sirve como stdout (es 1) de ls. Ejecutamos el comando ya dentro de f, ya que f es el stdout.


        int f; // abrimos file.txt como escritura!!
        f = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0600); //Creamos descriptor de escritura, cereación y truncamiento con respectivos permisos.
        // NO TENEMOS ERROR, SI NO EXISTE EL FICHERO, LO CREAMOS!!

        dup2(f,STDOUT_FILENO); // A partir de aquí, el stdout de lo que se ejecute va a ir a parar a f. A line->redirect

        if (execvp(line->commands->argv[0],line->commands->argv) < 0){ //Ejecuta el comando.
            //Error
            printf("%s: No se encuentra el mandato.\n",line->commands->argv[0]);
            exit(-1);

        }

    }

    if (line->redirect_error){

        int f; // abrimos gile.txt como escritura!!
        f = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600); //Creamos descriptor de archivo solo de lectura.
        // NO TENEMOS ERROR, SI NO EXISTE EL FICHERO, LO CREAMOS!!

        dup2(f,STDOUT_FILENO);

        if (execvp(line->commands->argv[0],line->commands->argv) < 0){ //Ejecuta el comando.
            //Error
            printf("%s: No se encuentra el mandato.\n",line->commands->argv[0]);
            exit(-1);

        }

    }

}

// 7
void mycd(tline *line){

    if ( line->commands->argc > 1){ // cd dir
        int dir = chdir(line->commands->argv[1]); // Change director!!
        int errnum;

        if (dir == -1){ // No existe directorio
            errnum = errno;
            fprintf(stderr,"%s: Error: %s\n", line->commands->argv[1],strerror(errnum)); // Imprimo "Error: No such file or directory."
        }
    } else{ //cd
        chdir(getenv("HOME"));
    }
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
            /*
            else if (line->redirect_input || line->redirect_output || line->redirect_error) {
                redirectionProcess(line);
            }
             */

            // Si se ejecuta en background:
            else if (line->background) {
                printf("Comando a ejecutarse en background\n");
                backgroundCommand(pid);
            }

                // Diferente número de comandos introducidos:
            else if (line->ncommands == 1){ // Añadir cd.
                //printf("Un solo comando introducido:\n");
                oneCommandProcess(line); // Procesamos 1 solo comando en la función con x mandatos.
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
