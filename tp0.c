#include "tp0.h"
#include <errno.h>


//como las agrego yo, no las paso al header
#define ID_RESPUESTA_VARIABLE 18
void loggearErrorYSalir(const char* mensaje);
void recibir(int socket, char* buffer, int tamanio_buffer, char* mensaje_error);

//como configure_logger retorna void, logger se define global
t_log* logger;

int main() {
  configure_logger();
  int socket = connect_to_server(IP, PUERTO);
  wait_hello(socket);
  Alumno alumno = read_hello();
  send_hello(socket, alumno);
  void * content = wait_content(socket);
  send_md5(socket, content);
  wait_confirmation(socket);

  //ojo! nunca se cierra el socket!

  exit_gracefully(0);
}

void configure_logger() {
  /*
	1.  Creemos el logger con la funcion de las commons log_create.
		Tiene que: guardarlo en el archivo tp0.log, mostrar 'tp0' al loggear,
		mostrarse por pantalla y mostrar solo los logs de nivel info para arriba
		(info, warning y error!)
  */
  logger = log_create("tp0.log", "tp0", true, LOG_LEVEL_INFO);
}

int connect_to_server(char * ip, char * port) {
  struct addrinfo hints;
  struct addrinfo *server_info;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
  hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	// Carga en server_info los datos de la conexion
  int estadoGetAddrInfo = getaddrinfo(ip, port, &hints, &server_info);
  if(estadoGetAddrInfo != 0){
	  //revisar esto
  	loggearErrorYSalir(gai_strerror(estadoGetAddrInfo));
  }  

  // 2. Creemos el socket con el nombre "server_socket" usando la "server_info" que creamos anteriormente
  //la Beej recomienda usar aca AF_INET
  int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
  if (server_socket < 0){
	perror("Problema con socket()");
  	loggearErrorYSalir("Problema con socket()");
  }

  // 3. Conectemosnos al server a traves del socket! Para eso vamos a usar connect()
  //chequear por que en la guia Beej se hace:  socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
  int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);
  freeaddrinfo(server_info);  // No lo necesitamos mas

  /*
	3.1 Recuerden chequear por si no se pudo contectar (usando el retorno de connect()).
		Si hubo un error, lo loggeamos y podemos terminar el programa con la funcioncita
		exit_gracefully pasandole 1 como parametro para indicar error ;).
		Pss, revisen los niveles de log de las commons.
  */

   if(retorno != 0){
      fprintf(stderr, "socket() failed: %s\n", strerror(errno));
   	close(server_socket);
  	loggearErrorYSalir("Problema con connect()");
  }

  // 4 Logeamos que pudimos conectar y retornamos el socket
   //comentado porque no muestra bien y rompe lo que sigue
  log_info(logger, "Conectado!");

  return server_socket;
}

void  wait_hello(int socket) {
  char * hola = "SYSTEM UTNSO 0.1";
  int tamanio_buffer = strlen(hola) + 1;

  /*
	5.  Ya conectados al servidor, vamos a hacer un handshake!
		Para esto, vamos a, primero recibir un mensaje del
		servidor y luego mandar nosotros un mensaje.
		Deberìamos recibir lo mismo que está contenido en la
		variable "hola". Entonces, vamos por partes:
		5.1.  Reservemos memoria para un buffer para recibir el mensaje.
  */
  char * buffer = calloc(tamanio_buffer, sizeof(char));

  /*
		5.2.  Recibamos el mensaje en el buffer.
		Recuerden el prototipo de recv:
		conexión - donde guardar - cant de bytes - flags(si no se pasa ninguno puede ir NULL)
		Nota: Palabra clave MSG_WAITALL.
  */

  recibir(socket, buffer, tamanio_buffer, "No se pudo recibir el mensaje de handshake. El servidor cerró la conexión?");

  log_info(logger, "Recibimos mensaje de handshake");

  if(strcmp(hola, buffer) != 0){
	//esta bien liberar aca? estoy repitiendo el free...
  	//una funcion que reciba n parametros con su funcion de liberar, y los libere?

  	free(buffer);
  	close(socket);
  	loggearErrorYSalir("Falló el handshake!!!");
  }

  log_info(logger, "Handshake OK!");

  free(buffer);
}

Alumno read_hello() {
  /*
	6.    Ahora nos toca mandar a nosotros un mensaje de hola.
		  que van a ser nuestros datos, definamos una variable de tipo Alumno.
		  Alumno es esa estructura que definimos en el .h.
		  Recuerden definir el nombre y apellido como cadenas varias, dado
		  que como se va a enviar toda la estructura completa, para evitar problemas
		  con otros otros lenguajes
  */

	//por que se definen como cadenas vacias? no entendi. que otros lenguajes?
  Alumno alumno = { .nombre = "", .apellido = "" };

  /*
	7.    Pero como conseguir los datos? Ingresemoslos por consola!
		  Para eso, primero leamos por consola usando la biblioteca realine.
		  Vamos a recibir, primero el legajo, despues el nombre y
		  luego el apellido
  */


  //agrega el \0 al final?
  char * legajo = readline("Legajo: ");
  char* nombre = readline("Nombre: ");
  char* apellido = readline("Apellido: ");

  /*
	8.    Realine nos va a devolver un cacho de memoria ya reservada
		  con lo que leyo del teclado hasta justo antes del enter (/n).
		  Ahora, nos toca copiar el legajo al la estructura alumno. Como
		  el legajo es numero, conviertanlo a numero con la funcion atoi
		  y asignenlo.
		  Recuerden liberar la memoria pedida por readline con free()!
  */

  //validar longitud de nombre y apellido? hasta 39 bytes

  alumno.legajo = atoi(legajo);

  //por que no queremos almacenar el \0 al final de la cadena? (si lo hicieramos, podriamos usar strcpy!)
  //si hacemos el memcpy solo con strlen, no se va a copiar el \0, si lo tuviera (mas abajo se menciona que lo tiene!)

  //pd: no se chequea el memcpy? devuelve void*
  memcpy(alumno.nombre, nombre, strlen(nombre));
  memcpy(alumno.apellido, apellido, strlen(apellido));

  free(legajo);
  free(nombre);
  free(apellido);

  /*
	9.    Para el nombre y el apellido no hace falta convertirlos porque
		  ambos son cadenas de caracteres, por los que solo hace falta
		  copiarlos usando memcpy a la estructura y liberar la memoria
		  pedida por readline.
  */

  // Usemos memcpy(destino, origen, cant de bytes).
  // Para la cant de bytes nos conviene usar strlen dado que son cadenas
  // de caracteres que cumplen el formato de C (terminar en \0)

  // 9.1. Faltaría armar el del apellido

  // 10. Finalmente retornamos la estructura
  return alumno;
}

void send_hello(int socket, Alumno alumno) {
  log_info(logger, "Enviando info de Estudiante");
  /*
	11.   Ahora SI nos toca mandar el hola con los datos del alumno.
		  Pero nos falta algo en nuestra estructura, el id_mensaje del protocolo.
		  Segun definimos, el tipo de id para un mensaje de tamaño fijo con
		  la informacion del alumno es el id 99
  */

  alumno.id_mensaje = 99;

  /*
	11.1. Como algo extra, podes probar enviando caracteres invalidos en el nombre
		  o un id de otra operacion a ver que responde el servidor y como se
		  comporta nuestro cliente.
  */  

  // alumno.id_mensaje = 33;
  // alumno.nombre[2] = -4;

  /*
	12.   Finalmente, enviemos la estructura por el socket!
		  Recuerden que nuestra estructura esta definida como __attribute__((packed))
		  por lo que no tiene padding y la podemos mandar directamente sin necesidad
		  de un buffer y usando el tamaño del tipo Alumno!
  */

  //se podria delegar ya que se usa mas abajo para mandar el md5
  int resultado = send(socket, &alumno, sizeof(Alumno), 0);
  //hay que preguntar por == -1, o con esto alcanza?
  if(resultado < 0){
  	//tengo que liberar el socket aca? de vuelta voy a repetir el close
  	close(socket);
  	loggearErrorYSalir("Problema con send() en send_hello");
  }

  /*
	12.1. Recuerden que al salir tenemos que cerrar el socket (ademas de loggear)!
  */

  //ojo! no se va a poder usar despues!!!!!
  //close(socket);
  log_info(logger, "Se enviaron los datos del estudiante");
}

void * wait_content(int socket) {
  /*
	13.   Ahora tenemos que recibir un contenido de tamaño variable
		  Para eso, primero tenemos que confirmar que el id corresponde al de una
		  respuesta de contenido variable (18) y despues junto con el id de operacion
		  vamos a haber recibido el tamaño del contenido que sigue. Por lo que:
  */

  int tamanio_header;
  //revisar esto: funciona bien el formateo?
  log_info(logger, "Esperando el encabezado del contenido(%ld bytes)", sizeof(ContentHeader));
  // 13.1. Reservamos el suficiente espacio para guardar un ContentHeader

  ContentHeader * header = malloc(sizeof(ContentHeader));

  // 13.2. Recibamos el header en la estructura y chequiemos si el id es el correcto.
  //      No se olviden de validar los errores, liberando memoria y cerrando el socket!

  //chequear el tercer parametro, el sizeof
  //mensaje_error un poco mas expresivo?
  //ojo que no se libera la memoria de header en caso de fallar el recibir!
  recibir(socket, header, sizeof(ContentHeader), "No se pudo recibir el header en wait_content()");

  //revisar el nombre de la constante, abajo se usa otra!
  if(header->id != ID_RESPUESTA_VARIABLE){
 	//deberia cerrar el socket?
  	close(socket);
  	free(header);
  	loggearErrorYSalir("wait_content() no recibio un mensaje de tamanio variable");
  }

  log_info(logger, "Esperando el contenido (%d bytes)", header->len);

  /*
	  14.   Ahora, recibamos el contenido variable. Ya tenemos el tamaño,
			por lo que reecibirlo es lo mismo que veniamos haciendo:
	  14.1. Reservamos memoria
	  14.2. Recibimos el contenido en un buffer (si hubo error, fallamos, liberamos y salimos
  */

  void* content = malloc(header->len);
  //ojo que aca tampoco se liberan las cosas, si rompe recibir
  recibir(socket, content, header->len, "No se pudo recibir el contenido en wait_content()");
  //para que los md5 coincidan!
  //content[(header->len) - 1] = '\0';
  /*
	  15.   Finalmente, no te olvides de liberar la memoria que pedimos
			para el header y retornar el contenido recibido.
  */

  free(header);

  log_info(logger, "Se recibio el mensaje de tamanio variable");

  return content;
}

void send_md5(int socket, void * content) {
  /*
	16.   Ahora calculemos el MD5 del contenido, para eso vamos
		  a armar el digest:
  */

  void * digest = malloc(MD5_DIGEST_LENGTH);
  MD5_CTX context;
  MD5_Init(&context);
  //se esta suponiendo que content es un string...
  MD5_Update(&context, content, strlen(content) + 1);
  MD5_Final(digest, &context);

  /*
	17.   Luego, nos toca enviar a nosotros un contenido variable.
		  A diferencia de recibirlo, para mandarlo es mejor enviarlo todo de una,
		  siguiendo la logida de 1 send - N recv.
		  Asi que:
  */

  //      17.1. Creamos un ContentHeader para guardar un mensaje de id 33 y el tamaño del md5

  //esta ok el tamanio del md5?
  ContentHeader header = { .id = 33, .len =  MD5_DIGEST_LENGTH };

  /*
		  17.2. Creamos un buffer del tamaño del mensaje completo y copiamos el header y la info de "digest" allí.
		  Recuerden revisar la función memcpy(ptr_destino, ptr_origen, tamaño)!
  */

  int message_size = sizeof(ContentHeader) + MD5_DIGEST_LENGTH;
  void * buffer = malloc(message_size);
  memcpy(buffer, &header, sizeof(ContentHeader));
  memcpy(buffer + sizeof(ContentHeader), digest, MD5_DIGEST_LENGTH);

  //esta validacion no es necesaria, pero la dejo por la pregunta que surge
  if(!buffer){
  	close(socket);

  	//ojo! aca tampoco se va a estar liberando la memoria de content
  	loggearErrorYSalir("No se pudo alocar el buffer para el md5 a enviar en send_md5");
  }

  /*
	18.   Con todo listo, solo nos falta enviar el paquete que armamos y liberar la memoria que usamos.
		  Si, TODA la que usamos, eso incluye a la del contenido del mensaje que recibimos en la función
		  anterior y el digest del MD5. Obviamente, validando tambien los errores.
  */

  int resultado = send(socket, buffer, sizeof(MD5_Final), 0);
  //hay que preguntar por == -1, o con esto alcanza?
  if(resultado < 0){
  	
  	//voy a repetir los free...
  	free(content);
  	free(digest);

  	free(buffer);

  	//tengo que liberar el socket aca? de vuelta voy a repetir el close
  	close(socket);
  	loggearErrorYSalir("Problema con send() en send_md5");
  }

  log_info(logger, "Se envio el MD5");

  free(content);
  free(digest);
  free(buffer);
}

void wait_confirmation(int socket) {
  int result = 0; // Dejemos creado un resultado por defecto. Yo lo cambie de 1 a 0,
  //ya que 1 es el valor correcto
  /*
	19.   Ahora nos toca recibir la confirmacion del servidor.
		  Si el resultado obvenido es distinto de 0, entonces hubo un error
  */

  //recibir(socket, &result, sizeof(int), "No se pudo recibir la confirmacion del envio de MD5");


  int result_recv = recv(socket, &result, sizeof(int), 0);

  if(result_recv <= 0){
	  close(socket);
	  loggearErrorYSalir("No se pudo recibir la confirmacion del MD5");
  }

  if(result != 1){
	  close(socket);
	  loggearErrorYSalir("Los MD5 no coincidieron");
  }

  log_info(logger, "Los MD5 concidieron!");
}

void exit_gracefully(int return_nr) {
  /*
	20.   Siempre llamamos a esta funcion para cerrar el programa.
		  Asi solo necesitamos destruir el logger y usar la llamada al
		  sistema exit() para terminar la ejecucion
  */

	log_destroy(logger);
	exit(return_nr);
}

void loggearErrorYSalir(const char* mensaje){
	log_error(logger, mensaje);
	exit_gracefully(1);
}

void recibir(int socket, char* buffer, int tamanio_buffer, char* mensaje_error){
	//hara falta hacer tamanio_buffer + 1, por si llega a mandarse con un \n al final? no

  //OJO! No tiene el wait all porque no mostraba en la consola. tema de hilos?
  int result_recv = recv(socket, buffer, tamanio_buffer, NULL);

  /*
		5.3.  Chequiemos errores al recibir! (y logiemos, por supuesto)
		5.4.  Comparemos lo recibido con "hola".
			  Pueden usar las funciones de las commons para comparar!
		No se olviden de loggear y devolver la memoria que pedimos!
		(si, también si falló algo, tenemos que devolverla, atenti.)
  */

  if(result_recv <= 0){

	if (buffer){
		free(buffer);
	}

	perror("Fallo el recv");
  	close(socket);
  	loggearErrorYSalir(mensaje_error);
  }
}
