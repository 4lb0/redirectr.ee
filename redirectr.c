#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE (1024 * 4) // 4 KB
#define SERVER_POLL_DELAY 100
#define DEFAULT_SUBDOMAIN "*"
#define MAX_SUBDOMAINS 100
#define MAX_SUB_LEN 50
#define MAX_URL_LEN 200
#define RESPONSE                                                               \
  "HTTP/1.1 301 Moved Permanently\r\nLocation: %s\r\n"                         \
  "Content-Length: 0\r\n\r\n"

char config_host[MAX_SUB_LEN] = "127.0.0.1";
int config_port = 8080;
int server = -1;
static volatile int8_t keep_running = 1;
int total_subdomains = 0;
char subdomains[MAX_SUBDOMAINS][MAX_SUB_LEN];
char redirects[MAX_SUBDOMAINS][MAX_URL_LEN];
char default_redirect[MAX_URL_LEN];

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
  if (bytes_read < 0) {
    perror("🚨 Error al leer el request");
    close(client_socket);
    return;
  }
  buffer[bytes_read] = '\0';
  char *host_header = strstr(buffer, "Host: ");
  char host[MAX_SUB_LEN] = {0};
  if (host_header) {
    host_header += 6; // Ignora "Host: "
    int i = 0;
    while (*host_header != '\r' && *host_header != '\n' &&
           *host_header != ' ' && *host_header != ':' && i < MAX_SUB_LEN - 1)
      host[i++] = *host_header++;
    host[i] = '\0';
  }
  char *redirect_url = default_redirect;
  for (int i = 0; i < total_subdomains; i++) {
    if (strcmp(subdomains[i], host) == 0) {
      redirect_url = redirects[i];
      break;
    }
  }
  char res[BUFFER_SIZE];
  int response_length = snprintf(res, sizeof(res), RESPONSE, redirect_url);
  printf("🌟 Redirige %s a %s\n", host, redirect_url);
  write(client_socket, res, response_length);
  close(client_socket);
}

int start_server(void) {
  server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    perror("🚨 Error al crear el socket");
    exit(EXIT_FAILURE);
  }
  int opt = 1;
  if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("🚨 Error al configurar el socket");
    close(server);
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = inet_addr(config_host),
      .sin_port = htons(config_port),
  };
  if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("🚨 El puerto ya esta en uso");
    close(server);
    exit(EXIT_FAILURE);
  }
  if (listen(server, 10) == -1) {
    perror("🚨 Error en el listen");
    close(server);
    exit(EXIT_FAILURE);
  }
  printf("🚀 Arrancando el servidor en http://%s:%d\n", config_host,
         config_port);
  return 0;
}

void stop_server(void) {
  if (server != -1) {
    close(server);
    server = -1;
    printf("👋 Chau!\n");
  }
}

int server_poll(int delay) {
  struct pollfd fds[1];
  fds[0].fd = server;
  fds[0].events = POLLIN;
  int poll_result = poll(fds, 1, delay);
  if (poll_result < 0) {
    if (server != -1) {
      perror("🚨 Error en el poll");
    }
    return -1;
  } else if (poll_result == 0) {
    return 0; // Timeout
  }
  if (fds[0].revents & POLLIN) {
    int client_socket = accept(server, NULL, NULL);
    if (client_socket == -1) {
      perror("🚨 Error al aceptar la conexión");
      return -1;
    }
    handle_client(client_socket);
  }
  return 0;
}

void sigintHandler(int signum) {
  (void)signum;
  keep_running = 0;
  stop_server();
}

void parse_config(char *config) {
  while (*config != '\0') {
    while (*config == ' ' || *config == '\t' || *config == '\n')
      config++; // Ignora espacios
    if (*config == '#' || *config == '\0') {
      while (*config != '\n' && *config != '\0')
        config++;
      continue;
    }
    char *sub_start = config;
    int sub_len = 0;
    while (*config != '=' && *config != '\0' && *config != '\n' &&
           *config != ' ' && *config != '\t' && sub_len < MAX_SUB_LEN - 1) {
      sub_len++;
      config++;
    }
    while (*config == ' ' || *config == '\t')
      config++; // Ignora espacios
    if (*config != '=')
      continue; // No es un subdominio
    strncpy(subdomains[total_subdomains], sub_start, sub_len);
    subdomains[total_subdomains][sub_len] = '\0';
    config++; // Ignora '='
    while (*config == ' ' || *config == '\t')
      config++; // Ignora espacios
    char *url_start = config;
    int url_len = 0;
    while (*config != '\0' && *config != '\n' && url_len < MAX_URL_LEN - 1) {
      url_len++;
      config++;
    }
    strncpy(redirects[total_subdomains], url_start, url_len);
    redirects[total_subdomains][url_len] = '\0';
    if (strcmp(subdomains[total_subdomains], "*") == 0) {
      strcpy(default_redirect, redirects[total_subdomains]);
    } else if (strcmp(subdomains[total_subdomains], "port") == 0) {
      config_port = atoi(redirects[total_subdomains]);
      continue;
    } else if (strcmp(subdomains[total_subdomains], "host") == 0) {
      strcpy(config_host, redirects[total_subdomains]);
      continue;
    }
    if (*config != '\0')
      config++;
    total_subdomains++;
    if (total_subdomains >= MAX_SUBDOMAINS)
      break;
  }
}

int main(void) {
  char config_data[BUFFER_SIZE];
  size_t bytes_read = read(STDIN_FILENO, config_data, BUFFER_SIZE - 1);
  if (bytes_read <= 0) {
    fprintf(stderr, "🚨 Error al leer la configuracion\n");
    return EXIT_FAILURE;
  }
  config_data[bytes_read] = '\0';
  parse_config(config_data);
  struct sigaction sa = { .sa_handler = sigintHandler, .sa_flags = 0 };
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
  start_server();
  while (keep_running)
    server_poll(SERVER_POLL_DELAY);
  stop_server();
  return EXIT_SUCCESS;
}
