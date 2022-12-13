
//Tipo de dato command:
typedef struct {
	char * filename; // Ruta completa del mandato (bash). Si no existe el mandato es NULL.
	int argc; // ¿Numero de mandatos?
	char ** argv; // ¿Array de mandatos?
} tcommand;


//Tipo de dato line:
typedef struct {
	int ncommands; // Número de mandatos de la linea
	tcommand * commands; // Array de tcommand (arriba)
	char * redirect_input; // Si la ENTRADA tiene redirección, puntero a string con nombre de redirección. Sino a NULL.
	char * redirect_output; // Igual que el anterior pero para redirección de SALIDA.
	char * redirect_error; // Igual que el anterior pero para redirección de ERROR.
	int background; // 1 en background (&), 0 si no.
} tline;


// Función:
// Recibe puntero a string y devuelve un puntero a tline con toda la información (arriba).
extern tline * tokenize(char *str);
