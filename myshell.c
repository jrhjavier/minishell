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

// setpgid: Parent group id.

typedef struct {
    pid_t pid;
    int status;
}job;

int num_bg = 0;
job array_bg[1024];


void prompt();
void oneCommandProcess(tline *line);
void moreTwoCommandProcess(tline* line);
void redirectionProcess(tline *line);
void cdCommand(tline *line);
void jobsCommand(tline *line);
void exitCommand();
void handler();


void handler() {
    pid_t temp;
    while ((temp = waitpid((pid_t) -1, NULL, WNOHANG)) > 0) {
        for (int i = 0; i < 1024; i++) {
            if (array_bg[i].pid == temp) {
                array_bg[i].status = 0;
                printf("\n");
                printf("Proceso: [%d] acabado\n", temp);
            }
        }
    }
}


void prompt(){
    char *p = getenv("USER");
    printf("%s_msh > ",p);
}


void oneCommandProcess(tline *line){
    // COMANDO CD
    if (strcmp(line->commands->argv[0],"cd") == 0){  // Comando cd
        cdCommand(line);
    }

    // COMANDO JOBS
    else if ((strcmp(line->commands->argv[0], "jobs") == 0)) { // Comando jobs
        jobsCommand(line);
    }

    // COMANDO EXIT
    else if (strcmp(line->commands[0].argv[0], "exit") == 0){  // comprueba si se pasa exit
        exitCommand();
    }

    // OTRO COMANDO
    else {

        pid_t pid;
        pid = fork();
        int status;

        if (pid < 0) {  // Error

            fprintf(stderr, "Fallo en el fork(). %s\n", strerror(errno));

        }

        else if (pid == 0) {  // Hijo

            redirectionProcess(line);

            if (line->background){

                signal(SIGINT,SIG_IGN);  // Ignoramos la señal Ctrl + C
                signal(SIGQUIT,SIG_IGN);  // Ignoramos la señal Ctrl + "\"

            }

            if (execvp(line->commands->argv[0], line->commands->argv) < 0) {  // Ejecuta el comando.
                // Error
                printf("%s: No se encuentra el mandato.\n", line->commands->argv[0]);
                exit(-1);
            }

        }

        else {  // Padre

            if(line->background){
                num_bg++;
                array_bg[num_bg].pid = pid;
                array_bg[num_bg].status = 1;
            }

            else{
                array_bg[num_bg].status = 0;
                wait(&status);
            }
        }
    }
}


void moreTwoCommandProcess(tline *line){

    int **matrix_pipes = (int**) malloc((line->ncommands) * sizeof(int*)); // Tantos pipes como comandos. Inutilizamos el 0.
    pid_t *array_pid = malloc(line->ncommands * sizeof(pid_t)); // Tantos procesos como comandos.
    pid_t pid;


    for (int i=0; i<=line->ncommands; i++){
        matrix_pipes[i] = (int *) malloc(2 * sizeof(int *));
        pipe(matrix_pipes[i]);
    }

    for (int i=0; i<line->ncommands; i++){

        pid = fork();
        array_pid[i] = pid;
        int status;

        if (pid < 0) {
            fprintf(stderr, "Fallo en el fork(). %s\n", strerror(errno));
            exit(-1);
        }

        if (pid == 0){  // Hijo

            //Cerramos todos los pipes menos el siguiente de escritura y el anterior de lectura:
            for (int j=0; j<=line->ncommands; j++){
                if(j != i){
                    close(matrix_pipes[j][0]);
                }
                if(j != i + 1){
                    close(matrix_pipes[j][1]);
                }
            }

            if (line->background){

                signal(SIGINT,SIG_IGN);  // Ignoramos la señal Ctrl + C
                signal(SIGQUIT,SIG_IGN);  // Ignoramos la señal Ctrl + "\"

            }

            // PRIMER COMANDO
            if (i == 0){
                redirectionProcess(line);
                close(matrix_pipes[i][0]);  // Cierro el pipe 0 lectura que quedará inutilizado -> no hay nada que leer
                dup2(matrix_pipes[i+1][1],STDOUT_FILENO);  // Salida estandar a pipe+1 de lectura.
            }

            // COMANDO INTERMEDIO
            else if(i>0 && i<line->ncommands-1){
                dup2(matrix_pipes[i][0],STDIN_FILENO);  // Leemos lo anterior
                dup2(matrix_pipes[i+1][1],STDOUT_FILENO);  // Lo redirigimos al siguiente

            }

            // ULTIMO COMANDO
            else{
                redirectionProcess(line);
                dup2(matrix_pipes[i][0],STDIN_FILENO);  // Leemos lo anterior
            }

            // Ejecutamos:
            execvp(line->commands[i].filename, line->commands[i].argv);

        }

        else{  // Padre
            if(line->background){
                num_bg++;
                array_bg[num_bg].pid = pid;
                array_bg[num_bg].status = 1;
            }
            else{
                array_bg[num_bg].status = 0;
                close(matrix_pipes[i][0]);
                close(matrix_pipes[i][1]);
                wait(&status);
            }
        }
    }
}


void redirectionProcess(tline *line){

    if (line->redirect_input) {

        int f; // abrimos file.txt como escritura!!
        int errnum; // Guardar numero de error
        f = open(line->redirect_input, O_RDONLY); //Creamos descriptor de archivo solo de lectura.

        if ( f != -1){
            dup2(f,STDIN_FILENO); // La stdin de lo que ejecutemos en adelante será lo que contiene el descriptor f.
            close(f);

        }else{

            errnum = errno; // Guardo numero de error
            fprintf(stderr,"%s: Error: %s\n", line->redirect_input, strerror(errnum)); // Imprimo "Error: No such file or directory."

        }
    }

    if (line->redirect_output) {

        int f; // abrimos file.txt como escritura!!
        f = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0664); //Creamos descriptor de escritura, cereación y truncamiento con respectivos permisos.
        // NO TENEMOS ERROR, SI NO EXISTE EL FICHERO, LO CREAMOS!!

        dup2(f,STDOUT_FILENO); // A partir de aquí, el stdout de lo que se ejecute va a ir a parar a f. A line->redirect
        close(f);
    }

    if (line->redirect_error){
        int f; // abrimos file.txt como escritura!!
        f = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600); //Creamos descriptor de archivo solo de lectura.
        // NO TENEMOS ERROR, SI NO EXISTE EL FICHERO, LO CREAMOS!!

        dup2(f,STDOUT_FILENO);
        close(f);

    }
}


void cdCommand(tline *line){

    if ( line->commands->argc > 1){  // cd dir
        int dir = chdir(line->commands->argv[1]);  // Change director
        int errnum;

        if (dir == -1){ // No existe directorio
            errnum = errno;
            fprintf(stderr,"%s: Error: %s\n", line->commands->argv[1],strerror(errnum)); // Imprimo "Error: No such file or directory."
        }
    } else{  // cd
        chdir(getenv("HOME"));
    }

}


void jobsCommand(tline *line){

    for (int i = 0; i < 5; i++) {
        if (array_bg[i+1].status==1) { // Si es bg.
            fprintf(stderr, "JOBS:\n");
            fprintf(stderr, "[%d]\n", array_bg[i + 1].pid);
        }
    }

}


void exitCommand(){

    fprintf(stderr, "\n");
    fprintf(stderr, "Hasta pronto!\n");
    fprintf(stderr, "\n");
    exit(0);

}


/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////

int main(void) {
    char buf[1024];
    tline * line;

    signal(SIGINT,SIG_IGN);  // Ignoramos señal Ctrl + C
    signal(SIGQUIT,SIG_IGN);  // Ctrl + "\"
    signal(SIGCHLD,handler);

    prompt();

    while (fgets(buf, 1024, stdin)) {

        line = tokenize(buf);  //Cogemos una línea por teclado.

        if (line==NULL) {  // Si está vacía continuamos hasta mostrar prompt.
            continue;
        }

        // 1 COMANDO
        if (line->ncommands == 1){ // Añadir cd.
            oneCommandProcess(line);  // Procesamos 1 solo comando en la función con x argumentos.
        }

        // 2 O MAS COMANDOS
        else if(line->ncommands >= 2){
            moreTwoCommandProcess(line);
        }

        prompt();

    }

    return 0;
}

/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////
