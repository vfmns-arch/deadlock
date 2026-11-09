#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>

#define CUSTOMER_FILE "customer.txt"
#define COMMAND_FILE "command.txt"
#define RESULT_FILE "result.txt"

typedef struct {
  int customers;
  int resources;
  int *available
  int **maximum;
  int **allocation;
  int **need;
} Banker;

void error(const char *message){
  fprintf(stderr, "Error: %s\n", message);
  exit(EXIT_FAILURE);
}

int **create_matrix(int rows, int cols){
  int **matrix = malloc(rows * sizeof(int *));
  if (!matrix){
    error("Memory allocation failed.");
  }
  for(int i = 0; i < rows; i++){
    matrix[i] = calloc(cols, sizeof(int));
    if(!matrix[i]){
    error("Memory allocation failed.");
  }
  return matrix;
}

void free_matrix(int **matrix, int rows){
  if(!matrix){
    return;
  }
  for(int i = 0; i < rows; i++){
    free(matrix[i]);
  }
  free(matrix);
  return;
}

void print_vector(FILE *out, const int *v, int n){
  for(int i = 0; i < n; i++){
    fprintf(out, "%s%d", i ? " " : "", v[i]);
  }
  return;
}

char *trim(char *s){
  while(isspace((unsigned char)*s)){
    s++;
  }
  char *end = s + strlen(s);
  while(end > s && isspace((unsigned char)end[-1])){
    *--end = '\0';
  }
  return s;
}

int parse_integer(const char *s, int *value){
  char *end;
  long n;
  if(!*s){
    return 0;
  }
  n = strtol(s, &end, 10);
  if(*end != '\0' || n < 0 || n > INT_MAX){
    return 0;
  }
  *value = (int)n;
  return 1;
}

void read_customers(Banker *b){
  FILE *file = fopen(CUSTOMER_FILE, "r");
  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;
  int count = 0;
  if(!file){
    error("Cannot open customer.txt");
  }
  while((length = getline(&line, &capacity, file)) != -1){
    char *p = trim(line);
    if(*p == '\0'){
      free(line);
      fclose(file);
      error("Empty line in customer.txt.");
    }
    int *values = calloc(b->resources, sizeof(int));
    if(!values){
      error("Memory allocation failed");
    }
    for(int j = 0; j < b->resources; j++){
      char *comma = strchr(p, ',');
      if(j < b->resources - 1 && !comma){
        free(values);
        free(line);
        fclose(file);
        error("Invalid number of resources in customer.txt.");
      }
      if(j == b->resources - 1 && comma){
        free(values);
        free(line);
        fclose(file);
        error("Too many resources in customer.txt.");
      }
      if(comma){
        *comma = '\0';
      }
      char *token = trim(p);
      if(!parse_integer(token, &values[j])){
        free(values);
        free(line);
        fclose(file);
        error("Invalid resource value in customer.txt.");
      }
      if(comma){
        p = comma + 1;
      }
    }
    if(count == INT_MAX){
      free(values);
      free(line);
      fclose(file);
      error("Too many customers.");
    }
    int **new_matrix = realloc(b->maximum, (count + 1) * sizeof(int *));
    if(!new_matrix){
      free(values);
      free(line);
      fclose(file);
      error("Memory allocation failed.");
    }
    b->maximum = new_matrix;
    b->maximum[count++] = values;
  }
  if(ferror(file)){
    free(line);
    fclose(file);
    error("Error reading customer.txt.");
  }
  free(line);
  fclose(file);
  if(count == 0){
    error("customers.txt has no numbers.");
  }
  b->customers = count;
  b->allocation = create_matrix(count, b->resources);
  b->need = create_matrix(count, b->resources);
  for(int i = 0; i < count; i++){
    for(int j = 0; j < b->resources; j++){
      b->need[i][j] = b->maximum[i][j];
    }
  }
  return;
}

int is_safe(Banker *b){
  int *work = malloc(b->resources * sizeof(int));
  int *finish = calloc(b->customers, sizeof(int));
  if(!work || !finish){
    error("Memory allocation failed.");
  }
  for(int j = 0; j < b->resources; j++){
    work[j] = b->available[j];
  }
  int completed = 0;
  int progress;
  do{
    progress = 0;
    for(int i = 0; i < b->customers; i++){
      if(finish[i]){
        continue;
      }
      int possible = 1;
      for(int j = 0; j < b->resources; j++){
        if(b->need[i][j] > work[j]){
          possible = 0;
          break;
        }
      }
      if(possible){
        for(int j = 0; j < b->resources; j++){
          work[j] += b->allocation[i][j];
        }
        finish[i] = 1;
        completed++;
        progress = 1;
      }
    }
  } while(progress);
  free(work);
  free(finish);
  return completed == b->customers;
}

void print_state(FILE *out, Banker *b){
  fprintf(out, "MAXIMUM | ALLOCATION | NEED\n");
  for(int i = 0; i < b->customers; i++){
    print_vector(out, b->maximum[i], b->resources);
    fprintf(out, " | ");
    print_vector(out, b->allocation[i], b->resources);
    fprintf(out, " | ");
    print_vector(out, b->need[i], b->resources);
    fprintf(out, "\n");
  }
  fprintf(out, "AVAILABLE ");
  print_vector(out, b->available, b->resources);
  fprintf(out, "\n");
  return;
}

void execute_command(Banker *b, FILE *out, const *char operation, int customer, int *request){
  if(customer < 0 || customer >= b->customers){
    error("Invalid customer index in commands.txt.");
  }
  if(strcmp(operation, "RQ") == 0){
    int exceeds = 0;
    int insufficient = 0;
    for(int j = 0; j < b->resources; j++){
      if(request[j] > b->need[customer][j]){
        exceeds = 1;
      }
      if(request[j] > b->available[j]){
        insufficient = 1;
      }
    }
    if(exceeds){
      fprintf(out, "The customer %d request ", customer);
      print_vector(out, request, b->resources);
      fprintf(out, " was denied because exceed its maximum allocation\n");
      return;
    }
    if(insufficient){
      fprintf(out, "The resources ");
      print_vector(out, b->available, b->resources);
      fprintf(out, " are not enough to customer %d request ", customer);
      print_vector(out, request, b->resources);
      fprintf(out, "\n");
      return;
    }
    for(int j = 0; j < b->resources; j++){
      b->available[j] -= request[j];
      b->allocation[customer][j] += request[j];
      b->need[customer][j] -= request[j];
    }
    if(is_safe(b)){
      fprintf(out, "Allocate to customer %d the resources ", customer);
      print_vector(out, request, b->resources);
      fprintf(out, "\n");
    } else {
      for(int j = 0; j < b->resources; j++){
        b->available[j] += request[j];
        b->allocation[customer][j] -= request[j];
        b->need[customer][j] += request[j];
      }
      fprintf(out, "The customer %d request ", customer);
      print_vector(out, request, b->resources);
      fprintf(out, " was denied because result is an unsafe state\n");
    }
  } else if(strcmp(operation, "RL") == 0){
    for(int j = 0; j < b->resources; j++){
      if(request[j] > b->allocation[customer][j]){
        error("Release exceeds customer allocation.");
      }
    }
    for(int j = 0; j < b->resources; j++){
      b->available[j] += request[j];
      b->allocation[customer][j] -= request[j];
      b->need[customer][j] += request[j];
    }
    fprintf(out, "Release from customer %d the resources ", customer);
    print_vector(out, request, b->resources);
    fprintf(out, "\n");
  } else {
    error("Unknown command in commands.txt.");
  }
  return;
}

void read_command(Banker *b){
  FILE *file = fopen(COMMAND_FILE, "r");
  FILE *out = fopen(RESULT_FILE, "w");
  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;
  if(!file){
    error("Cannot open commands.txt.");
  }
  if(!out){
    fclose(file);
    error("Cannot create result.txt.");
  }
  while((length = getline(&line, &capacity, file)) != -1){
    char *p = trim(line);
    if(*p == '\0'){
      free(line);
      fclose(file);
      fclose(out);
      error("Empty line in commands.txt.");
    }
    if(strcmp(p, "*") == 0){
      fprintf(out, "\n");
      print_state(out, b);
      continue;
    }
    char *tokens[b->resources + 3];
    int token_count = 0;
    char *saveptr;
    char *token = strtok_r(p, " \t\r\n", &saveptr);
    while(token){
      if(token_count >= b->resources + 3){
        free(line);
        fclose(file);
        fclose(out);
        error("Invalid command format in commands.txt.");
      }
      tokens[token_count++] = token;
      token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    if(token_count != b->resources + 2){
      free(line);
      fclose(file);
      fclose(out);
      error("Incorrect number of arguments in commands.txt.");
    }
    if(strcmp(tokens[0], "RQ") != 0 && strcmp(tokens[0], "RL") != 0){
      free(line);
      fclose(file);
      fclose(out);
      error("Invalid operation in commands.txt.");
    }
    int customer;
    if(!parse_integer(tokens[1], &customer)){
      free(line);
      fclose(file);
      fclose(out);
      error("Invalid customer index.");
    }
    int *request = malloc(b->resources * sizeof(int));
    if(!request){
      free(line);
      fclose(file);
      fclose(out);
      error("Memory allocation failed.");
    }
    for(int j = 0; j < b->resources; j++){
      if(!parse_integer(tokens[j + 2], &request[j])){
        free(request);
        free(line);
        fclose(file);
        fclose(out);
        error("Invalid resource value in commands.txt.");
      }
    }
    execute_command(b, out, tokens[0], customer, request);
    free(request);
  }
  if(ferror(file)){
    free(line);
    fclose(file);
    fclose(out);
    error("Error reading commands.txt.");
  }
  free(line);
  fclose(file);
  fclose(out);
  return;
}

int main(int argc, char *argv[])
{
    Banker b = {0};

    if (argc < 2)
        error("Usage: ./banker resource1 resource2 ...");

    b.resources = argc - 1;

    b.available = malloc(b.resources * sizeof(int));

    if (!b.available)
        error("Memory allocation failed.");

    for (int i = 0; i < b.resources; i++) {
        if (!parse_integer(argv[i + 1], &b.available[i])) {
            free(b.available);
            error("Invalid resource argument.");
        }
    }

    read_customers(&b);
    read_commands(&b);

    free(b.available);
    free_matrix(b.maximum, b.customers);
    free_matrix(b.allocation, b.customers);
    free_matrix(b.need, b.customers);

    return EXIT_SUCCESS;
}
