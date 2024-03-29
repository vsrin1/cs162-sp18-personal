#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "libhttp.h"
#include "wq.h"

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue;
int num_threads;
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;
pthread_t* thread_arr;
time_t start_time;

/* Forward declearion */
typedef struct fd_pair {
  int from;
  int to;
} fd_pair;
void* proxy_child_thread_work(void* arg);

/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd) {

  /*
   * TODO: Your solution for Task 1 goes here! Feel free to delete/modify *
   * any existing code.
   */
  static int served = 0;
  struct http_request *request = http_request_parse(fd);
  if (request == NULL) {
    close(fd);
    return;
  }
  struct stat s;
  char fullpath[MAX_PATH];
  strcpy(fullpath, server_files_directory);
  strcat(fullpath, request->path);
  if (stat(fullpath, &s) != 0) {
    printf("file not found\n");
    http_start_response(fd, 404);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    http_send_string(fd,
        "<center>"
        "<h1>FILE NOT FOUND!</h1>"
        "<hr>"
        "<p>Nothing's here yet.</p>"
        "</center>");
    close(fd);
    http_request_free(request);
    return;
  }

  if (S_ISDIR(s.st_mode)) {
    printf("Serving directory '%s':\n", request->path);
    http_start_response(fd, 200);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);

    char content[MAX_FILE_SIZE];
    size_t n = http_get_list_files(server_files_directory, request->path, content, MAX_FILE_SIZE);
    http_send_data(fd, content, n);
  } else if (S_ISREG(s.st_mode)) {
    printf("Serving file '%s':\n", request->path);
    http_start_response(fd, 200);
    http_send_header(fd, "Content-Type", http_get_mime_type(fullpath));
    http_end_headers(fd);

    char content[MAX_FILE_SIZE];
    int fin = open(fullpath, O_RDONLY);
    size_t n = read(fin, content, MAX_FILE_SIZE);
    close(fin);
    http_send_data(fd, content, n);
  }
  time_t t;
  time(&t);
  printf("Finish serving. Total served: %i. Time: %lf\n", ++served, difftime(t, start_time));
  close(fd);
  http_request_free(request);
}


/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream client_socket_fd and the
 * proxy target. HTTP requests from the client (client_socket_fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (client_socket_fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int client_socket_fd) {

  /*
  * The code below does a DNS lookup of server_proxy_hostname and 
  * opens a connection to it. Please do not modify.
  */

  struct sockaddr_in target_address;
  memset(&target_address, 0, sizeof(target_address));
  target_address.sin_family = AF_INET;
  target_address.sin_port = htons(server_proxy_port);

  struct hostent *target_dns_entry = gethostbyname2(server_proxy_hostname, AF_INET);

  int server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (server_socket_fd == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  if (target_dns_entry == NULL) {
    fprintf(stderr, "Cannot find host: %s\n", server_proxy_hostname);
    exit(ENXIO);
  }

  char *dns_address = target_dns_entry->h_addr_list[0];

  memcpy(&target_address.sin_addr, dns_address, sizeof(target_address.sin_addr));
  int connection_status = connect(server_socket_fd, (struct sockaddr*) &target_address,
      sizeof(target_address));

  if (connection_status < 0) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(client_socket_fd);

    http_start_response(client_socket_fd, 502);
    http_send_header(client_socket_fd, "Content-Type", "text/html");
    http_end_headers(client_socket_fd);
    http_send_string(client_socket_fd, "<center><h1>502 Bad Gateway</h1><hr></center>");
    return;

  }

  /* 
  * TODO: Your solution for task 3 belongs here! 
  */

  /* Threading pooling is not implemented */
  /* TODO: implement threading pooling */
  /* Create a child thread for client->server connection */
  pthread_t thread_sc;
  pthread_create(&thread_sc, NULL, proxy_child_thread_work, &(fd_pair){ .from = client_socket_fd, .to = server_socket_fd });
  /* Create a child thread for server->client connection */
  pthread_t thread_cs;
  pthread_create(&thread_cs, NULL, proxy_child_thread_work, &(fd_pair){ .from = server_socket_fd, .to = client_socket_fd });
  /* Wait for child thread to finish */
  pthread_join(thread_cs, NULL);
  pthread_join(thread_sc, NULL);
  printf("Finish handling proxy\n");
}

void* proxy_child_thread_work(void* arg) {
  int thread = (unsigned int)(pthread_self() % 100);
  printf("thread: %i\tstart proxy \n", thread);
  int from_fd = ((fd_pair*)arg)->from;
  int to_fd = ((fd_pair*)arg)->to;

  ssize_t size = MAX_FILE_SIZE;
  char buffer[size];

  while ((size = read(from_fd, buffer, MAX_FILE_SIZE)) > 0) {
    printf("thread: %i\treads size: %li\n", thread, size);
    size = write(to_fd, buffer, size);
    printf("thread: %i\twrites size: %li\n", thread, size);
  }
  close(from_fd);
  close(to_fd);
  printf("thread: %i\tend proxy \n", thread);
  return NULL;
}

void* worker_work(void* arg) {
  void (*request_handler)(int) = arg;
  pthread_mutex_lock(&work_queue.lock);
  while(1) {
    printf("queue size:%i\tthread id: %i\n", work_queue.size, (unsigned int)(pthread_self() % 100));
    if (work_queue.shutdown) {
      pthread_mutex_unlock(&work_queue.lock);
      break;
    } else if (work_queue.size > 0) {
      int fd = wq_pop(&work_queue);
      pthread_mutex_unlock(&work_queue.lock);
      request_handler(fd);
      close(fd);
    } else {
      pthread_cond_wait(&work_queue.cv, &work_queue.lock);
    }
  }
  return NULL;
}

void init_thread_pool(int num_threads, void (*request_handler)(int)) {
  /*
   * TODO: Part of your solution for Task 2 goes here!
   */
  pthread_t* ptr = thread_arr = malloc(sizeof(pthread_t) * num_threads);
  for (int i = 0; i < num_threads; ++i) {
    pthread_create(ptr++, NULL, worker_work, request_handler);
  }
  printf("%i threads created\n", num_threads);
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  init_thread_pool(num_threads, request_handler);

  while (1) {
    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    if (num_threads == 0) {
      request_handler(client_socket_number);
      close(client_socket_number);
    } else {
      pthread_mutex_lock(&work_queue.lock);
      wq_push(&work_queue, client_socket_number);
      pthread_cond_signal(&work_queue.cv);
      pthread_mutex_unlock(&work_queue.lock);
    }

    

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  /* Exit worker threads */
  if (num_threads > 0)  {
    pthread_mutex_lock(&work_queue.lock);
    work_queue.shutdown = 1;
    pthread_cond_broadcast(&work_queue.cv);
    pthread_mutex_unlock(&work_queue.lock);
    for (int i = 0; i < num_threads; ++i) {
      pthread_join(thread_arr[i], NULL);
    }
    free(thread_arr); 
  }
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000 [--num-threads 5]\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000 [--num-threads 5]\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  num_threads = 0;
  void (*request_handler)(int) = NULL;
  time(&start_time);

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char *num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
