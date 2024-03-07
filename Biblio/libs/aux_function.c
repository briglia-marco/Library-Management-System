#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

// Funzioni ausiliarie

void free_libro(Libro_t *libro){
  if(libro == NULL){
    return;
  }
  if(libro->autore != NULL){
    safe_free(libro->autore);
  }
  if(libro->titolo != NULL){
    safe_free(libro->titolo);
  }
  if(libro->editore != NULL){
    safe_free(libro->editore);
  }
  if(libro->nota != NULL){
    safe_free(libro->nota);
  }
  if(libro->collocazione != NULL){
    safe_free(libro->collocazione);
  }
  if(libro->luogo_pubblicazione != NULL){
    safe_free(libro->luogo_pubblicazione);
  }
  if(libro->descrizione_fisica != NULL){
    safe_free(libro->descrizione_fisica);
  }
  if(libro->prestito != NULL){
    safe_free(libro->prestito);
  }
  if(libro->volume != NULL){
    safe_free(libro->volume);
  }
  if(libro->scaffale != NULL){
    safe_free(libro->scaffale);
  }
  safe_free(libro);
}

void free_biblioteca(linked_list_t *biblioteca){
  mypthread_mutex_lock(&biblioteca->lock, __LINE__, __FILE__);
  node_t *current = biblioteca->head;
  node_t *next = NULL;
  while(current != NULL){
    free_libro((Libro_t *)current->data);
    current = current->next;
  }
  while(current != NULL){
    next = current->next;
    safe_free(current);
    current = next;
  }
  safe_free(current);
  mypthread_mutex_unlock(&biblioteca->lock, __LINE__, __FILE__);
  safe_free(biblioteca);
}

void riempi_scheda_libro(Libro_t *libro, char *line){

  char *etichetta = strtok(line, ":");
  char *valore = strtok(NULL, ";");
  remove_spaces(etichetta);
  remove_spaces(valore);

  while((etichetta != NULL) && (valore != NULL)){
    
    if(strcmp(etichetta, "autore") == 0){
      if(libro->autore == NULL){
        libro->autore = (char *)malloc(strlen(valore) + 1);
        if(libro->autore == NULL){
          perror("Errore allocazione memoria");
          exit(EXIT_FAILURE);
        }
        remove_spaces(valore);
        strcpy(libro->autore, valore);
      }
      else{
        libro->autore = (char *)realloc(libro->autore, strlen(libro->autore) + strlen(valore) + 3);
        if(libro->autore == NULL){
          perror("Errore riallocazione memoria");
          exit(EXIT_FAILURE);
        }
        strcat(libro->autore, "; autore: ");
        remove_spaces(valore);
        strcat(libro->autore, valore);
      }
    }
    
    if(strcmp(etichetta, "titolo") == 0){
      libro->titolo = (char *)malloc(strlen(valore) + 1);
      if(libro->titolo == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->titolo, valore);
    }
    
    if(strcmp(etichetta, "editore") == 0){
      libro->editore = (char *)malloc(strlen(valore) + 1);
      if(libro->editore == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->editore, valore);
    }
    
    if(strcmp(etichetta, "anno") == 0){
      libro->anno = atoi(valore);
    }
    
    if(strcmp(etichetta, "nota") == 0){
      libro->nota = (char *)malloc(strlen(valore) + 1);
      if(libro->nota == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->nota, valore);
    }
    
    if(strcmp(etichetta, "collocazione") == 0){
      libro->collocazione = (char *)malloc(strlen(valore) + 1);
      if(libro->collocazione == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->collocazione, valore);
    }
    
    if(strcmp(etichetta, "luogo_pubblicazione") == 0){
      libro->luogo_pubblicazione = (char *)malloc(strlen(valore) + 1);
      if(libro->luogo_pubblicazione == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->luogo_pubblicazione, valore);
    }
    
    if(strcmp(etichetta, "descrizione_fisica") == 0){
      libro->descrizione_fisica = (char *)malloc(strlen(valore) + 1);
      if(libro->descrizione_fisica == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->descrizione_fisica, valore);
    }
    
    if(strcmp(etichetta, "prestito") == 0){
      libro->prestito = (char *)malloc(strlen(valore) + 1);
      if(libro->prestito == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->prestito, valore);
    }
    
    if(strcmp(etichetta, "volume") == 0){
      libro->volume = (char *)malloc(strlen(valore) + 1);
      if(libro->volume == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->volume, valore);
    }
    
    if(strcmp(etichetta, "scaffale") == 0){
      libro->scaffale = (char *)malloc(strlen(valore) + 1);
      if(libro->scaffale == NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
      }
      strcpy(libro->scaffale, valore);
    }

    etichetta = strtok(NULL, ":");
    valore = strtok(NULL, ";");
    remove_spaces(etichetta);
    remove_spaces(valore);
  }
  
}

void remove_spaces(char* str){
    if(str == NULL)
      return;
    int end = 0, start = 0;
    int length = strlen(str);
    if(length==0){
      return;
    }
    while(isspace(str[start])){
        start++;
    }
    end = length - 1;
    while(isspace(str[end]) && end > start){
        end--;
    }
    int i, j = 0;
    for(i=start;i<=end;i++){
        str[j] = str[i];
        j++;
    }
    str[j] = '\0';
}

void stampa_libro(Libro_t *libro) {
    if(libro == NULL) {
        printf("Il libro non esiste.\n");
        return;
    }
    printf("Autore: %s\n", libro->autore);
    printf("Titolo: %s\n", libro->titolo);
    printf("Editore: %s\n", libro->editore);
    printf("Anno: %d\n", libro->anno);
    printf("Nota: %s\n", libro->nota);
    printf("Collocazione: %s\n", libro->collocazione);
    printf("Luogo pubblicazione: %s\n", libro->luogo_pubblicazione);
    printf("Descrizione fisica: %s\n", libro->descrizione_fisica);
    printf("Prestito: %s\n", libro->prestito);
    printf("Volume: %s\n", libro->volume);
    printf("Scaffale: %s\n", libro->scaffale);
}

void print_biblioteca(linked_list_t *biblioteca){
  mypthread_mutex_lock(&biblioteca->lock, __LINE__, __FILE__);
  node_t *current = biblioteca->head;
  while (current != NULL){
    stampa_libro((Libro_t *)current->data);
    printf("____________NUOVO LIBRO____________\n");
    current = current->next;
  }
  mypthread_mutex_unlock(&biblioteca->lock, __LINE__, __FILE__);
}

int is_in_biblioteca(linked_list_t *biblioteca, Libro_t *libro){
  mypthread_mutex_lock(&biblioteca->lock, __LINE__, __FILE__);
  node_t *current = biblioteca->head; //punto al primo libro
  while (current != NULL){
    if(compara_libri((Libro_t *)current->data, libro) == 1){ // se i libri sono uguali ritorno 1
      mypthread_mutex_unlock(&biblioteca->lock, __LINE__, __FILE__);
      return 1;
    }
    current = current->next; // se sono diversi passo al prossimo libro 
  }
  mypthread_mutex_unlock(&biblioteca->lock, __LINE__, __FILE__);
  return 0;
}

int compara_libri(Libro_t *libro1, Libro_t *libro2){
  if(libro1 == NULL || libro2 == NULL){
    return 0;
  }
  if(libro1->autore != NULL && libro2->autore != NULL){
    if(strcmp(libro1->autore, libro2->autore) != 0){
      return 0;
    }
  }
  if(libro1->titolo != NULL && libro2->titolo != NULL){
    if(strcmp(libro1->titolo, libro2->titolo) != 0){
      return 0;
    }
  }
  if(libro1->editore != NULL && libro2->editore != NULL){
    if(strcmp(libro1->editore, libro2->editore) != 0){
      return 0;
    }
  }
  if(libro1->anno != libro2->anno){
    return 0;
  }
  if(libro1->nota != NULL && libro2->nota != NULL){
    if(strcmp(libro1->nota, libro2->nota) != 0){
      return 0;
    }
  }
  if(libro1->collocazione != NULL && libro2->collocazione != NULL){
    if(strcmp(libro1->collocazione, libro2->collocazione) != 0){
      return 0;
    }
  }
  if(libro1->luogo_pubblicazione != NULL && libro2->luogo_pubblicazione != NULL){
    if(strcmp(libro1->luogo_pubblicazione, libro2->luogo_pubblicazione) != 0){
      return 0;
    }
  }
  if(libro1->descrizione_fisica != NULL && libro2->descrizione_fisica != NULL){
    if(strcmp(libro1->descrizione_fisica, libro2->descrizione_fisica) != 0){
      return 0;
    }
  }
  if(libro1->prestito != NULL && libro2->prestito != NULL){
    if(strcmp(libro1->prestito, libro2->prestito) != 0){
      return 0;
    }
  }
  if(libro1->volume != NULL && libro2->volume != NULL){
    if(strcmp(libro1->volume, libro2->volume) != 0){
      return 0;
    }
  }
  if(libro1->scaffale != NULL && libro2->scaffale != NULL){
    if(strcmp(libro1->scaffale, libro2->scaffale) != 0){
      return 0;
    }
  }

  return 1;
}

void remove_all_spaces(char *str){
  int count = 0;
  for(int i = 0; str[i]; i++){
    if(str[i] != ' '){
      str[count++] = str[i];
    }
  }
  str[count] = '\0';
}

void inizializza_libro(Libro_t *libro){
  libro->autore = NULL;
  libro->titolo = NULL;
  libro->editore = NULL;
  libro->anno = 0;
  libro->nota = NULL;
  libro->collocazione = NULL;
  libro->luogo_pubblicazione = NULL;
  libro->descrizione_fisica = NULL;
  libro->prestito = NULL;
  libro->volume = NULL;
  libro->scaffale = NULL;
}

void inizializza_richiesta(richiesta_client_t *richiesta){
  richiesta->etichetta = NULL;
  richiesta->valore = NULL;
}

void check_richiesta(char *token, char *string, int var_controllo, linked_list_t *lista_arg){
  if(strstr(token, string) != NULL){   
    if(var_controllo){               
      perror("DUPLICATE ARGUMENTS");  
      exit(EXIT_FAILURE);              
    }                                
    add_richiesta(lista_arg, token); 
  }  
}

// restituisce 1 se biosgna fare un prestito dei record richiesti, 0 altrimenti
int parsing_client(int argc, char *argv[], linked_list_t *lista_arg){
  // salvo su una lista i valori delle opzioni e dei valori corrispondenti
  int aut = 0, tit = 0, edit = 0, year = 0, note = 0, coll = 0, luog = 0, descr = 0, vol = 0, scaff = 0, prestito = 0;
  char *token;
  for(int i = 1; i<argc; i++){
    token = argv[i];
    remove_spaces(token);
    if(token == NULL){
      perror("NO ARGUMENTS");
      exit(1);
    }
    if(strcmp(token, "-p") == 0){ 
      prestito++;
    }
    check_richiesta(token, "--autore", aut, lista_arg);
    check_richiesta(token, "--titolo", tit, lista_arg);
    check_richiesta(token, "--editore", edit, lista_arg);
    check_richiesta(token, "--anno", year, lista_arg);
    check_richiesta(token, "--nota", note, lista_arg);
    check_richiesta(token, "--collocazione", coll, lista_arg);
    check_richiesta(token, "--luogo_pubblicazione", luog, lista_arg);
    check_richiesta(token, "--descrizione_fisica", descr, lista_arg);
    check_richiesta(token, "--volume", vol, lista_arg);
    check_richiesta(token, "--scaffale", scaff, lista_arg);
  }
  return prestito;
}

void remove_dashes(char *str){
  int count = 0;
  for(int i = 0; str[i]; i++){
    if(str[i] != '-'){
      str[count++] = str[i];
    }
  }
  str[count] = '\0';
}

void to_upper_case(char *str){
  if(str == NULL){
    return;
  }
  if(str[0] >= 'a' && str[0] <= 'z'){
    str[0] = str[0] - 32;
  }
}

void add_richiesta(linked_list_t *lista_arg, char *token){
  char *etichetta, *valore;
  richiesta_client_t *richiesta = (richiesta_client_t *)malloc(sizeof(richiesta_client_t));
  inizializza_richiesta(richiesta);
  etichetta = strtok(token, "=");
  remove_dashes(etichetta);
  valore = strtok(NULL, "$");
  to_upper_case(valore);
  richiesta->etichetta = (char *)malloc(strlen(etichetta) + 1);
  richiesta->valore = (char *)malloc(strlen(valore) + 1);
  strcpy(richiesta->etichetta, etichetta);
  strcpy(richiesta->valore, valore);
  add_node(lista_arg, richiesta);
}

int fill_arr_socket(int arr_socket[], char arr_server[]){
  FILE *fd = myfopen(CONFIG_FILE, "r", __LINE__, __FILE__);
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char *server = NULL;
  char *socket = NULL;
  int i = 0;
  char *dup = NULL;
  while((read = getline(&line, &len, fd)) != -1){
    if(strstr(line, "SERVER")!=NULL){
      dup = strdup(line);
      server = strtok(dup, ":");
      server = strtok(NULL, ",");
      remove_spaces(server);
      arr_server[i] = *(char*)strdup(server);
    }
    if(strstr(line, "SOCKET")!=NULL){
      dup = strdup(line);
      socket = strtok(dup, ":");
      socket = strtok(NULL, ":");
      socket = strtok(NULL, "$");
      remove_spaces(socket);
      arr_socket[i] = atoi(socket);
    }
    i++;
  }
  myfclose(fd, __LINE__, __FILE__);
  return i;
}

int calcola_data_sec(char *data){
  struct tm tm;
  time_t t;
  strptime(data, "%Y-%m-%d %H:%M:%S", &tm);
  t = mktime(&tm);
  return t;
}

char *data_to_string(char buffer[]){
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  strftime(buffer, 20, "%d-%m-%Y %H:%M:%S", tm);
  return buffer;
}

void check_prestito(Libro_t *libro){
  if(libro->prestito != NULL){
    time_t t = calcola_data_sec(libro->prestito);
    int diff = difftime(time(NULL), t);
    if(diff > TEMPO_LIMITE_PRESTITO){ // se il tempo di prestito è scaduto
      libro->prestito = NULL; // rimuovo la data di prestito
    }
  }
}



