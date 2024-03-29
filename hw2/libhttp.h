/*
 * A simple HTTP library.
 *
 * Usage example:
 *
 *     // Returns NULL if an error was encountered.
 *     struct http_request *request = http_request_parse(fd);
 *
 *     ...
 *
 *     http_start_response(fd, 200);
 *     http_send_header(fd, "Content-type", http_get_mime_type("index.html"));
 *     http_send_header(fd, "Server", "httpserver/1.0");
 *     http_end_headers(fd);
 *     http_send_string(fd, "<html><body><a href='/'>Home</a></body></html>");
 *
 *     close(fd);
 */

#ifndef LIBHTTP_H
#define LIBHTTP_H

#define MAX_PATH 1024
#define MAX_FILE_SIZE 4096


/*
 * Functions for parsing an HTTP request.
 */
struct http_request {
  char *method;
  char *path;
};

struct http_request *http_request_parse(int fd);
void http_request_free(struct http_request* request);

/*
 * Functions for sending an HTTP response.
 */
void http_start_response(int fd, int status_code);
void http_send_header(int fd, char *key, char *value);
void http_end_headers(int fd);
void http_send_string(int fd, char *data);
void http_send_data(int fd, char *data, size_t size);

/*
 * Helper functions
 */
/* Gets the Content-Type based on a file name. */
char *http_get_mime_type(char *file_name);
/* Gets list of files from path in html */
size_t http_get_list_files(const char* http_files_dir, char* request_path, char* buff, size_t size);

#endif
