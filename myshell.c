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


typedef struct {
    pid_t pidd;
    pid_t pidg;
    int eliminado;
}job;

int num_bg = 0;
job array_bg[1024];


void prompt(); // 1
void oneCommandProcess(tline *line); // 2
void moreTwoCommandProcess(tline* line); // 4
void backgroundCommand(int pid); // 5
void redirectionProcess(tline *line); // 6
void mycd(tline *line); // 7


// 1
void prompt(){
    char *p = getenv("USER");
    printf("%s_msh >",p);
}


// 2
void oneCommandProcess(tline *line){
    pid_t pid;
    pid = fork();

    if (pid < 0){  // Error
        fprintf(stderr, "Fallo en el fork\n");
    }

    else if (pid == 0){  // Hijo
        if (line->background) {
            printf("PID: [%d]\n", pid);
            //array_bg[num_bg] = pid;

            num_bg++;
        }

        if (line->redirect_input || line->redirect_output || line->redirect_error ) {
            //printf("redirección de entrada: %s\n", line->redirect_input);
            redirectionProcess(line);
        }

        if (strcmp(line->commands->argv[0],"cd") == 0){  // Comando cd
            mycd(line);
        }

        else if ((strcmp(line->commands->argv[0],"jobs") == 0)){ // Comando jobs
            for(int i=0; i<1024; i++) {
                if (array_bg[i].eliminado == 0) {
                    printf("%d Running PID: %i\n", i + 1, array_bg[i].pidd); // Imprimimos TODOS nuestros procesos.
                }
            }
        }

        else { //CAMBIAR ELSE IF PORQUE SI METO JOBS ME SACA NO SE ENCUENTRA EL COMANDO. ¿y SI TIENE REDIRECCION?

            if (execvp(line->commands->argv[0], line->commands->argv) < 0) { //Ejecuta el comando.
                // Error
                printf("%s: No se encuentra el mandato.\n", line->commands->argv[0]);
                exit(-1);
            }
        }
    }

    else{  // Padre
        waitpid(pid, NULL, 0);
    }
}

// 4
void moreTwoCommandProcess(tline *line){
    int **matrix_pipes = (int**) malloc((line->ncommands) * sizeof(int*)); // Tantos pipes como comandos. Inutilizamos el 0.
    pid_t *array_pid = malloc(line->ncommands * sizeof(pid_t)); // Tantos procesos como comandos.
    pid_t pid;
    //int status;

    for (int i=0; i<=line->ncommands; i++){
        matrix_pipes[i] = (int *) malloc(2 * sizeof(int *));
        pipe(matrix_pipes[i]);
    }

    for (int i=0; i<=line->ncommands; i++){

        pid = fork();
        array_pid[i] = pid;

        if (pid < 0) {
            fprintf(stderr, "Falló el fork(). %s\n", strerror(errno));
            exit(-1);
        }

        if (pid == 0){  // Hijo
            //Cerramos todos los pipes menos el siguiente de escritura y el anterior de lectura.:
            for (int j=0; j<=line->ncommands; j++){
                if(j != i)
                    close(matrix_pipes[j][0]);
                if(j != i + 1)
                    close(matrix_pipes[j][1]);
            }

            // PRIMER COMANDO
            if (i == 0){
                redirectionProcess(line);
                close(matrix_pipes[i][0]);  // Cierro el pipe 0 lectura que quedará inutilizado -> no hay nada que leer
                dup2(matrix_pipes[i+1][1],STDOUT_FILENO);  // Salida estandar a pipe+1 de lectura.

            }

            // COMANDO INTERMEDIO
            else if(i<line->ncommands-1){
                dup2(matrix_pipes[i][0],STDIN_FILENO);  // Leemos lo anterior
                dup2(matrix_pipes[i+1][1],STDOUT_FILENO);  // Lo redirigimos al siguiente

            }

            // ULTIMO COMANDO
            else{
                // Redirigimos la entrada desde el pipe anterior.
                redirectionProcess(line);
                dup2(matrix_pipes[i][0],STDIN_FILENO);

            }

            // Ejecutamos:
            execvp(line->commands[i].filename, line->commands[i].argv);

        } else{  // Padre
            close(matrix_pipes[i][0]);
            close(matrix_pipes[i][1]);
            waitpid(pid, NULL, 0);
        }
    }
}


// 5
void  backgroundCommand(int pid){
    printf("Mostramos pid de proceso en background es: %d \n", pid);
}


// 6
void redirectionProcess(tline *line){
    if (line->redirect_input) {

        printf("ENTRADA");
        // Hay redirección de entrada.

        // Ejemplo: sort < .txt. Creamos descriptor de fichero f. Lee lo que hay en .txt y
        // dup2 copia f en 0, que sirve como stdin (es 0) de sort.

        int f; // abrimos gile.txt como escritura!!
        int errnum; // Guardar numero de error
        f = open(line->redirect_input, O_RDONLY); //Creamos descriptor de archivo solo de lectura.

        if ( f != -1){

            dup2(f,STDIN_FILENO); // La stdin de lo que ejecutemos en adelante será lo que contiene el descriptor f.

            /*
            if (execvp(line->commands->argv[0],line->commands->argv) < 0){ //Ejecuta el comando.
                //Error
                printf("%s: No se encuentra el mandato.\n",line->commands->argv[0]);
                exit(-1);
            }
             */

        }else{

            errnum = errno; // Guardo numero de error
            fprintf(stderr,"%s: Error: %s\n", line->redirect_input, strerror(errnum)); // Imprimo "Error: No such file or directory."

        }
    }

    else if (line->redirect_output) {
        printf("Hay redirección de salida.");

        // Ejemplo: ls > .txt. Creamos descriptor de fichero f. Puede escribir, crear y truncar lo que hay en .txt
        // dup2 copia f en 1, que sirve como stdout (es 1) de ls. Ejecutamos el comando ya dentro de f, ya que f es el stdout.

        printf("Redireccion de salida: %s", line->redirect_output);
        int f; // abrimos file.txt como escritura!!
        f = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0600); //Creamos descriptor de escritura, cereación y truncamiento con respectivos permisos.
        // NO TENEMOS ERROR, SI NO EXISTE EL FICHERO, LO CREAMOS!!

        dup2(f,STDOUT_FILENO); // A partir de aquí, el stdout de lo que se ejecute va a ir a parar a f. A line->redirect
        close(f);

        /*
        if (execvp(line->commands->argv[0],line->commands->argv) < 0){ //Ejecuta el comando.
            //Error
            printf("%s: No se encuentra el mandato.\n",line->commands->argv[0]);
            exit(-1);

        }
         */
    }

    if (line->redirect_error){
        int f; // abrimos file.txt como escritura!!
        f = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600); //Creamos descriptor de archivo solo de lectura.
        // NO TENEMOS ERROR, SI NO EXISTE EL FICHERO, LO CREAMOS!!

        dup2(f,STDOUT_FILENO);

        /*
        if (execvp(line->commands->argv[0],line->commands->argv) < 0){ //Ejecuta el comando.
            //Error
            printf("%s: No se encuentra el mandato.\n",line->commands->argv[0]);
            exit(-1);
        }
         */
    }
}

// 7
void mycd(tline *line){
    if ( line->commands->argc > 1){  // cd dir
        int dir = chdir(line->commands->argv[1]);  // Change director!!
        int errnum;

        if (dir == -1){ // No existe directorio
            errnum = errno;
            fprintf(stderr,"%s: Error: %s\n", line->commands->argv[1],strerror(errnum)); // Imprimo "Error: No such file or directory."
        }
    } else{  // cd
        chdir(getenv("HOME"));
    }
}


/////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////////////////////

int main(void) {
    char buf[1024];
    tline * line;
    //int status;

    for (int i=0; i < 1024; i++){
        array_bg[i].eliminado = 1;
    }

    prompt();

    while (fgets(buf, 1024, stdin)) {

        line = tokenize(buf); //Cogemos una línea por teclado.

        if (line==NULL) { // Si está vacía continuamos hasta mostrar prompt.
            continue;
        }

        // COMANDO EXIT
        if (line->ncommands == 1 && strcmp(line->commands[0].argv[0], "exit") == 0){  // comprueba si se pasa exit
            fprintf(stderr, "\n");
            fprintf(stderr, "Hasta pronto!\n");
            fprintf(stderr, "\n");
            exit(0);
        }

        // Si se ejecuta en background:
        if (line->background) {
            signal(SIGINT,SIG_IGN); // Ignoramos señal Ctrl + C
            signal(SIGQUIT,SIG_IGN);// Ctrl + "\"
            //backgroundCommand(pid);
            /*
            setpgid(0, 0);
            int f = open("/dev/null", O_RDONLY);
            dup2(f,STDIN_FILENO);
            */
        }

        else if(!line->background){
            signal(SIGINT, SIG_DFL); // Por defecto Ctrl + C
            signal(SIGQUIT, SIG_DFL); // Ctrl + "\"
        }

        // 1 COMANDO
        if (line->ncommands == 1){ // Añadir cd.
            oneCommandProcess(line);  // Procesamos 1 solo comando en la función con x mandatos.
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
