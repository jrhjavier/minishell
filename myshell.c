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
void moreTwoCommandProcess(pid_t pid, pid_t *pidHijos, int **pipes, tline* line){
    pidHijos = malloc(line->ncommands * sizeof (int));  // creamos tantos pids como comandos haya
    pipes = malloc((line->ncommands - 1) * sizeof(int *));  // reservamos memoria para matriz de pipes

    for (int i = 0; i < line->ncommands - 1; ++i) {  // creamos tantos pipes como comandos - 1 haya
        pipes[i] = (int *) malloc(sizeof (int) * 2);  // reservamos memoria (2 posiciones) para el pipe
        if(pipe(pipes[i]) < 0){  // inicializamos el pipe
            fprintf(stderr, "Error al crear el pipe %s\n", strerror(errno));
        }
    }

    for (int i = 0; i < line->ncommands; ++i) {  // un hijo por cada comando
        pid = fork();

        if(pid < 0){  // Error
            fprintf(stderr, "Error al crear al hijo. Fallo en el fork %s\n", strerror(errno));
            exit(-1);
        }

        else if(pid == 0){  // Hijo
            // PRIMER COMANDO
            if (i == 0){
                //printf("Soy el primer comando\n");
                if (line->redirect_input != NULL){  // Hay redireccion de entrada
                    ///// FUNCION REDIRECCION DE ENTRADA /////
                }
                for (int j = 1; j < line->ncommands - 1; ++j) {  // Como hemos creado tantos pipes, hay que cerrarlos menos el 0 (que es el que voy a usar)
                    for (int k = 0; k <= 1; ++k) {
                        close(pipes[j][k]);  // Cerramos entrada y salida (0 y 1)
                    }
                }
                close(pipes[0][0]);  // cerramos el de lectura del que vamos a usar ya que vamos a meterlo en el pipe
                dup2(pipes[0][1], 1);  // escribimos en el pipe
            }

            // COMANDO INTERMEDIO
            else if (0 < i < line->ncommands-1){
                //printf("Soy el comando intermedio %d\n", i);
                if(line->ncommands != 3){  // si no hay 3 comandos
                    if (i == 1){  // soy el SEGUNDO COMANDO y no soy el penultimo (ya que no hay 3 comandos)
                        for (int j = i+1; j < line->ncommands-1; ++j) {  // cerramos todos los pipes menos el que vamos a usar
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                    }

                    else if (i == line->ncommands-2){  // soy el PENULTIMO COMANDO
                        for (int j = 0; j < i-1; ++j) {  // cerramos todos los pipes anteriores al penultimo
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                    }

                    else{  // no soy NI el PENULTIMO NI el SEGUNDO
                        for (int j = 0; j < i-1; ++j) {  // cerramos todos los pipes anteriores al que vamos a utilizar
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                        for (int j = i+1; j < line->ncommands-1; ++j) {  // cerramos todos los pipes siguientes al que vamos a utilizar
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                    }
                }
                close(pipes[i-1][1]);  // cerramos la escritura del anterior al comando que toca
                dup2(pipes[i-1][0], 0);  // metemos lo que lea el comando anterior en el stdin del comando actual
                close(pipes[i][0]);  // cerramos la lectura del comando que toca
                dup2(pipes[i][1], 1);  // metemos la escritura del comando que toca en el stdout
            }

            // ULTIMO COMANDO
            else{
                //printf("Soy el ultimo comando\n");
                if (line->redirect_output != NULL){  // Hay redireccion de salida
                    ///// FUNCION REDIRECCION DE SALIDA /////
                }
                if (line->redirect_error != NULL){  // Hay redireccion de error
                    ///// FUNCION REDIRECCION DE ERROR /////
                }
                for (int j = 0; j < line->ncommands-2; ++j) {  // cerramos todos los comandos menos el que vamos a utilizar (todos los anteriores)
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                close(pipes[i-1][1]);  // cerramos la escritura del comando anterior
                dup2(pipes[i-1][0], 0);  // metemos la lectura del comando anterior en el stdin del ultimo comando
            }
            execvp(line->commands[i].filename, line->commands[i].argv);  // ejecutamos el comando con sus argumentos
            // Si llegamos hasta aqui -> no se ha ejecutado el comando correctamente (Error)
            fprintf(stderr, "Error al ejecutar el comando.\n");
            fprintf(stderr, "%s: No se encuentra el comando.\n", line->commands[i].filename);
        }

        else{  //Padre
            pidHijos[i] = pid;  // vamos guardando los pids de los hijos en el array de pids
        }
    }

    for (int i = 0; i < line->ncommands; ++i) {  // esperamos a TODOS los hijos
        waitpid(pidHijos[i], NULL, 0);  // -1 -> espera por cualquier proceso hijo -> NO SE SI HACE FALTA EL ARRAY DE PIDS!!!
    }

    for (int i = 0; i < line->ncommands-1; ++i) {  // cerramos todos los pipes utilizados
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < line->ncommands; ++i) {
        free(pipes[i]);  // liberamos la matriz de pipes
    }
    free(pipes);  // liberamos la matriz de pipes
    free(pidHijos);  // liberamos el array de pids

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
