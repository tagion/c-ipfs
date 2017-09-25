/**
 * Methods for lightweight/specific HTTP for API communication.
 */
#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/uio.h>

#include <fcntl.h>

#include "libp2p/net/p2pnet.h"
#include "libp2p/os/memstream.h"
#include "libp2p/utils/logger.h"
#include "ipfs/core/api.h"
#include "ipfs/importer/exporter.h"
#include "ipfs/core/http_request.h"

//pthread_mutex_t conns_lock;
//int conns_count;

//struct ApiContext api_list;

/**
 * Write two strings on one write.
 * @param fd file descriptor to write.
 * @param str1 first string to write.
 * @param str2 second string to write.
 */
size_t write_dual(int fd, char *str1, char *str2)
{
	struct iovec iov[2];

	iov[0].iov_base = str1;
	iov[0].iov_len = strlen(str1);
	iov[1].iov_base = str2;
	iov[1].iov_len = strlen(str2);

	return writev(fd, iov, 2);
}

int find_chunk(char *buf, const size_t buf_size, size_t *pos, size_t *size)
{
	char *p = NULL;

	*size = strtol(buf, &p, 16);
	if (!p || p < buf || p > (buf + 10)) {
		return 0;
	}
	*pos = (int)(p - buf);
	if (p[0] == '\r' && p[1] == '\n') {
		*pos += 2;
		return 1;
	}
	return 0;
}

int read_chunked(int fd, struct s_request *req, char *already, size_t already_size)
{
	char buf[MAX_READ], *p;
	size_t pos, nsize, buf_size = 0, r;

	if (already_size > 0) {
		if (already_size <= sizeof(buf)) {
			memcpy(buf, already, already_size);
			buf_size += already_size;
			already_size = 0;
		} else {
			memcpy(buf, already, sizeof(buf));
			already += sizeof(buf);
			buf_size += sizeof(buf);
			already_size -= sizeof(buf);
		}
	}

	while(buf_size) {
		if (!find_chunk(buf, buf_size, &pos, &nsize)) {
			libp2p_logger_error("api", "fail find_chunk.\n");
			libp2p_logger_error("api", "nsize = %d.\n", nsize);
			return 0;
		}
		if (nsize == 0) {
			break;
		}
		p = realloc(req->buf, req->size + nsize);
		if (!p) {
			libp2p_logger_error("api", "fail realloc.\n");
			return 0;
		}
		req->buf = p;
		req->size += nsize;

CPCHUNK:
		r = nsize;
		buf_size -= pos;
		if (r > buf_size) {
			r = buf_size;
		}
		memcpy(req->buf + req->body + req->body_size, buf + pos, r);
		req->body_size += r;
		nsize -= r;
		buf_size -= r;
		if (buf_size > 0) {
			memmove(buf, buf + pos + r, buf_size);
		}
		pos = 0;
		if (already_size > 0) {
			r = sizeof(buf) - buf_size;
			if (already_size <= r) {
				memcpy(buf, already, already_size);
				buf_size += already_size;
				already_size = 0;
			} else {
				memcpy(buf, already, r);
				already += r;
				buf_size += r;
				already_size -= r;
			}
		}

		if (socket_read_select4(fd, 5) > 0) {
			r = sizeof(buf) - buf_size;
			r = read(fd, buf+buf_size, r);
			buf_size += r;
			if (r == 0 && nsize == 0) {
				break;
			}

			if (r <= 0) {
				libp2p_logger_error("api", "read fail.\n");
				return 0;
			}
		}

		if (nsize > 0)
			goto CPCHUNK; // still have data to transfer on current chunk.

		if (memcmp (buf, "\r\n", 2)!=0) {
			libp2p_logger_error("api", "fail CRLF.\n");
			return 0;
		}
	}
	return 1;
}

int read_all(int fd, struct s_request *req, char *already, size_t alread_size)
{
	char buf[MAX_READ], *p;
	size_t size = 0;

	if (alread_size > 0) {
		p = realloc(req->buf, req->size + alread_size);
		if (!p) {
			return 0;
		}
		req->buf = p;
		req->size += alread_size;
		memcpy(req->buf + req->body + req->body_size, already, alread_size);
		req->body_size += alread_size;
	}
	for(;;) {
		if (socket_read_select4(fd, 5) <= 0) {
			break;
		}
		size = read(fd, buf, sizeof buf);
		if (size <= 0) {
			break;
		}
		p = realloc(req->buf, req->size + size);
		if (!p) {
			return 0;
		}
		req->buf = p;
		req->size += size;
		memcpy(req->buf + req->body + req->body_size, buf, size);
		req->body_size += size;
	}
	return 1;
}

/**
 * Find a token in a string array.
 * @param string array and token string.
 * @returns the pointer after where the token was found or NULL if it fails.
 */
char *str_tok(char *str, char *tok)
{
	char *p = strstr(str, tok);
	if (p) {
		p += strlen(tok);
		while(*p == ' ') p++;
	}
	return p;
}

/**
 * Find a token in a binary array.
 * @param array, size of array, token and size of token.
 * @returns the pointer after where the token was found or NULL if it fails.
 */
char *bin_tok(char *bin, size_t limit, char *tok, size_t tok_size)
{
	char *p = memmem(bin, limit, tok, tok_size);
	if (p) {
		p += tok_size;
	}
	return p;
}

/**
 * Check if header contain a especific value.
 * @param request structure, header name and value to check.
 * @returns the pointer where the value was found or NULL if it fails.
 */
char *header_value_cmp(struct s_request *req, char *header, char *value)
{
	char *p = str_tok(req->buf + req->header, header);
	if (p) {
		if (strstart(p, value)) {
			return p;
		}
	}
	return NULL;
}

/**
 * Lookup for boundary at buffer string.
 * @param body buffer string, boundary id, filename and content-type string.
 * @returns the pointer where the multipart start.
 */
char *boundary_find(char *str, char *boundary, char **filename, char **contenttype)
{
	char *p = str_tok(str, "--");
	while (p) {
		if (strstart(p, boundary)) {
			// skip to the beginning, ignoring the header for now, if there is.
			// TODO: return filename and content-type
			p = strstr(p, "\r\n\r\n");
			if (p) {
				return p + 4; // ignore 4 bytes CRLF 2x
			}
			break;
		}
		p = str_tok(str, "--");
	}
	return NULL;
}

/**
 * Return the size of boundary.
 * @param boundary buffer, boundary id.
 * @returns the size of boundary or 0 if fails.
 */
size_t boundary_size(char *str, char *boundary, size_t limit)
{
	char *p = bin_tok(str, limit, "\r\n--", 4);
	while (p) {
		if (strstart(p, boundary)) {
			if (cstrstart(p + strlen(boundary), "--\r\n")) {
				p -= 4;
				return (size_t)(p - str);
			}
		}
		p = bin_tok(p, limit, "\r\n--", 4);
	}
	return 0;
}

/**
 * function to find and read the object.
 * @param path is the ipfs address, obj is a pointer to be allocated and will be the return of the data, size is a pointer to return the data length.
 * @returns 1 when success is 0 when failure.
 */
int get_object(struct IpfsNode* local_node, char *path, unsigned char **obj, size_t *size)
{
	FILE* memstream_file = NULL;
	char* memstream_char = NULL;
	size_t memstream_size = 0;
	struct Cid* cid = NULL;

	// convert hash to cid
	if ( ipfs_cid_decode_hash_from_base58((unsigned char*)path, strlen(path), &cid) == 0) {
		return 0;
	}

	// find block
	struct HashtableNode* read_node = NULL;
	if (!ipfs_exporter_get_node(local_node, cid->hash, cid->hash_length, &read_node)) {
		ipfs_cid_free(cid);
		return 0;
	}

	// open a memory stream
	memstream_file = open_memstream(&memstream_char, &memstream_size);
	if (memstream_file == NULL) {
		libp2p_logger_error("api", "get_object: Unable to open a memory stream.\n");
		ipfs_cid_free(cid);
		return 0;
	}

	// throw everything (including links) into the memory stream
	ipfs_exporter_cat_node(read_node, local_node, memstream_file);

	fclose(memstream_file);

	// no longer need these
	ipfs_cid_free(cid);
	ipfs_hashtable_node_free(read_node);

	*size = memstream_size;
	*obj = (unsigned char*)memstream_char;

	return 1;
}

/**
 * send object data as an http response.
 * @param socket, object pointer and size.
 * @returns 1 when success is 0 when failure.
 */
int send_object(int socket, unsigned char *obj, size_t size)
{
	// TODO: implement.
	return 0; // fail.
}

struct ApiConnectionParam {
	int index;
	struct IpfsNode* this_node;
};

/***
 * Take an s_request and turn it into an HttpRequest
 * @param req the incoming s_request
 * @returns the resultant HttpRequest or NULL on error
 */
struct HttpRequest* api_build_http_request(struct s_request* req) {
	struct HttpRequest* request = ipfs_core_http_request_new();
	if (request != NULL) {
		char *segs = malloc (strlen(req->buf + req->request) + 1);
		if (segs) {
			strcpy(segs, req->buf + req->request);
			request->command = segs;
			segs = strchr(segs, '/');
			if (segs) {
				*segs++ = '\0';
				request->sub_command = segs; // sub_command can contain another level as filters/add
			}
			if (req->query) {
				segs = malloc (strlen(req->buf + req->query) + 1);
				if (segs) {
					strcpy(segs, req->buf + req->query);
					while (segs) {
						char *value, *name = segs;
						segs = strchr(segs, '&');
						if (segs) { // calc next to split before search for = on another parameter.
							*segs++ = '\0';
						}

						value = strchr(name, '=');
						if (value) {
							*value++ = '\0';
						}
						if (value && (strcmp(name, "arg")==0)) {
							libp2p_utils_vector_add(request->arguments, value);
						} else {
							struct HttpParam *hp = ipfs_core_http_param_new();
							if (hp) {
								hp->name = name;
								hp->value = value; // maybe null ?
								libp2p_utils_vector_add(request->params, hp);
							}
						}
					}
				}
			}
		}
	}
	return request;
}

/**
 * Pthread to take care of each client connection.
 * @param ptr an ApiConnectionParam
 * @returns nothing
 */
void *api_connection_thread (void *ptr)
{
	int timeout, s, r;
	struct ApiConnectionParam* params = (struct ApiConnectionParam*)ptr;
	char resp[MAX_READ+1], buf[MAX_READ+1], *p, *body;
	char client[INET_ADDRSTRLEN];
	struct s_request req;
	int (*read_func)(int, struct s_request*, char*, size_t) = read_all;

	req.buf = NULL; // sanity.

	buf[MAX_READ] = '\0';

	s = params->this_node->api_context->conns[params->index]->socket;
	timeout = params->this_node->api_context->timeout;

	if (socket_read_select4(s, timeout) <= 0) {
		libp2p_logger_error("api", "Client connection timeout.\n");
		goto quit;
	}
	r = read(s, buf, sizeof buf);
	if (r <= 0) {
		libp2p_logger_error("api", "Read from client fail.\n");
		goto quit;
	}
	buf[r] = '\0';

	p = strstr(buf, "\r\n\r\n");

	if (p) {
		body = p + 4;

		req.size = p - buf + 1;
		req.buf = malloc(req.size);
		if (!req.buf) {
			// memory allocation fail.
			libp2p_logger_error("api", "malloc fail.\n");
			write_cstr (s, HTTP_500);
			goto quit;
		}
		memcpy(req.buf, buf, req.size - 1);
		req.buf[req.size-1] = '\0';

		req.method = 0;
		p = strchr(req.buf + req.method, ' ');
		if (!p) {
			libp2p_logger_error("api", "fail looking for space on method '%s'.\n", req.buf + req.method);
			write_cstr (s, HTTP_400);
			goto quit;
		}
		*p++ = '\0'; // End of method.
		req.path = p - req.buf;
		if (strchr(p, '?')) {
			p = strchr(p, '?');
			*p++ = '\0';
			req.query = p - req.buf;
		} else {
			req.query = 0;
		}
		p = strchr(p, ' ');
		if (!p) {
			libp2p_logger_error("api", "fail looking for space on path '%s'.\n", req.buf + req.path);
			write_cstr (s, HTTP_400);
			goto quit;
		}
		*p++ = '\0'; // End of path.
		req.http_ver = p - req.buf;
		p = strchr(req.buf + req.http_ver, '\r');
		if (!p) {
			libp2p_logger_error("api", "fail looking for CR on http_ver '%s'.\n", req.buf + req.http_ver);
			write_cstr (s, HTTP_400);
			goto quit;
		}
		*p++ = '\0'; // End of http version.
		while (*p == '\r' || *p == '\n') p++;
		req.header = p - req.buf;
		req.body = req.size;
		req.body_size = 0;

		if (header_value_cmp(&req, "Transfer-Encoding:", "chunked")) {
			read_func = read_chunked;
		}

		if (!read_func(s, &req, body, r - (body - buf))) {
			libp2p_logger_error("api", "fail read_func.\n");
			write_cstr (s, HTTP_500);
			goto quit;
		}

		// once we leave the building of the req struct, do we need to do more? This flag will tell us.
		int further_processing_necessary = 0;

		if (strncmp(req.buf + req.method, "GET", 3)==0) {
			if (strcmp (req.buf + req.path, "/")==0      ||
			    strcmp (req.buf + req.path, "/webui")==0 ||
			    strcmp (req.buf + req.path, "/webui/")==0) {
				char *redir;
				size_t size = sizeof(HTTP_301) + (sizeof(WEBUI_ADDR)*2);

				redir = malloc(size);
				if (redir) {
					snprintf(redir, size, HTTP_301, WEBUI_ADDR, WEBUI_ADDR);
					redir[size-1] = '\0'; // just in case
					write_dual (s, req.buf + req.http_ver, strchr (redir, ' '));
					free (redir);
				} else {
					write_cstr (s, HTTP_500);
				}
			} else if (cstrstart(req.buf + req.path, API_V0_START)) {
				req.request = req.path + sizeof(API_V0_START) - 1;
				further_processing_necessary = 1;
			} else {
				// TODO: handle download file here.
				// move out of the if to do further processing
			}
			// end of GET
		} else if (strncmp(req.buf + req.method, "POST", 4)==0) {
			// TODO: Handle gzip/json POST requests.

			p = header_value_cmp(&req, "Content-Type:", "multipart/form-data;");
			if (p) {
				p = str_tok(p, "boundary=");
				if (p) {
					char *boundary, *l;
					int len;
					if (*p == '"') {
						p++;
						l = strchr(p, '"');
					} else {
						l = p;
						while (*l != '\r' && *l != '\0') l++;
					}
					len = l - p;
					boundary = malloc (len+1);
					if (boundary) {
						memcpy(boundary, p, len);
						boundary[len] = '\0';

						p = boundary_find(req.buf + req.body, boundary, NULL, NULL);
						if (p) {
							req.boundary_size = boundary_size(p, boundary, req.size - (p - buf));
							if (req.boundary_size > 0) {
								req.boundary = p - req.buf;
							}
						}

						free (boundary);
					}
				}
			}

			if (req.boundary > 0) {
				libp2p_logger_error("api", "boundary index = %d, size = %d\n", req.boundary, req.boundary_size);
			}

			libp2p_logger_debug("api", "method = '%s'\n"
						   "path = '%s'\n"
						   "http_ver = '%s'\n"
						   "header {\n%s\n}\n"
						   "body_size = %d\n",
			req.buf+req.method, req.buf+req.path, req.buf+req.http_ver,
			req.buf+req.header, req.body_size);
			// end of POST
		} else {
			// Unexpected???
			libp2p_logger_error("api", "fail unexpected '%s'.\n", req.buf + req.method);
			write_cstr (s, HTTP_500);
			further_processing_necessary = 0;
		}

		if (further_processing_necessary) {
			// now do something with the request we have built
			struct HttpRequest* http_request = api_build_http_request(&req);
			if (http_request != NULL) {
				char* response_text = NULL;
				if (!ipfs_core_http_request_process(params->this_node, http_request, &response_text)) {
					libp2p_logger_error("api", "ipfs_core_http_request_process returned false.\n");
					// TODO: Handle this condition
				}
				ipfs_core_http_request_free(http_request);
				// now send the results
				snprintf(resp, sizeof(resp), "%s 200 OK\r\n" \
				"Content-Type: application/json\r\n"
				"Server: c-ipfs/0.0.0-dev\r\n"
				"X-Chunked-Output: 1\r\n"
				"Connection: close\r\n"
				"Transfer-Encoding: chunked\r\n\r\n%s", req.buf + req.http_ver, response_text);
				if (response_text != NULL)
					free(response_text);
				write_str (s, resp);
				libp2p_logger_debug("api", "resp = {\n%s\n}\n", resp);
			} else {
				// uh oh... something went wrong converting to the HttpRequest struct
				libp2p_logger_error("api", "Unable to build HttpRequest struct.\n");
			}
		}
	} else {
		libp2p_logger_error("api", "fail looking for body.\n");
		write_cstr (s, HTTP_400);
	}

quit:
	if (req.buf)
		free(req.buf);
	if (inet_ntop(AF_INET, &( params->this_node->api_context->conns[params->index]->ipv4), client, INET_ADDRSTRLEN) == NULL)
		strcpy(client, "UNKNOW");
	libp2p_logger_debug("api", "Closing client connection %s:%d (%d).\n", client, params->this_node->api_context->conns[params->index]->port, params->index+1);
	libp2p_logger_debug("api", "api_connection_thread: Attempting lock of mutex.\n");
	pthread_mutex_lock(&params->this_node->api_context->conns_lock);
	libp2p_logger_debug("api", "api_connection_thread: Lock successful.\n");
	close(s);
	free ( params->this_node->api_context->conns[params->index]);
	params->this_node->api_context->conns[params->index] = NULL;
	params->this_node->api_context->conns_count--;
	pthread_mutex_unlock(&params->this_node->api_context->conns_lock);
	libp2p_logger_debug("api", "api_connection_thread: Unlock successful.\n");
	free(params);
	return NULL;
}

/**
 * Close all connections stopping respectives pthreads and free allocated memory.
 */
void api_connections_cleanup (struct IpfsNode* local_node)
{
	int i;

	libp2p_logger_debug("api", "api_connections_cleanup: Attempting lock.\n");
	pthread_mutex_lock(&local_node->api_context->conns_lock);
	libp2p_logger_debug("api", "api_connections_cleanup: Lock successful.\n");
	if (local_node->api_context->conns_count > 0 && local_node->api_context->conns) {
		for (i = 0 ; i < local_node->api_context->max_conns ; i++) {
			if (local_node->api_context->conns[i]->pthread) {
				pthread_cancel (local_node->api_context->conns[i]->pthread);
				close (local_node->api_context->conns[i]->socket);
				free (local_node->api_context->conns[i]);
				local_node->api_context->conns[i] = NULL;
			}
		}
		local_node->api_context->conns_count = 0;
	}
	if (local_node->api_context->conns) {
		free (local_node->api_context->conns);
		local_node->api_context->conns = NULL;
	}
	pthread_mutex_unlock(&local_node->api_context->conns_lock);
	libp2p_logger_debug("api", "api_connections_cleanup: Unlock successful\n");
}

/**
 * Pthread to keep in background dealing with client connections.
 * @param ptr is not used.
 * @returns nothing
 */
void *api_listen_thread (void *ptr)
{
	int s;
	INT_TYPE i;
	uint32_t ipv4;
	uint16_t port;
	char client[INET_ADDRSTRLEN];
	struct IpfsNode* local_node = (struct IpfsNode*)ptr;

	local_node->api_context->conns_count = 0;

	for (;;) {
		s = socket_accept4(local_node->api_context->socket, &ipv4, &port);
		if (s <= 0) {
			break;
		}
		if (local_node->api_context->conns_count >= local_node->api_context->max_conns) { // limit reached.
			libp2p_logger_error("api", "Limit of connections reached (%d).\n", local_node->api_context->max_conns);
			close (s);
			continue;
		}

		libp2p_logger_debug("api", "api_listen_thread: Lock\n");
		pthread_mutex_lock(&local_node->api_context->conns_lock);
		libp2p_logger_debug("api", "api_listen_thread: Lock Complete\n");
		for (i = 0 ; i < local_node->api_context->max_conns && local_node->api_context->conns[i] ; i++);
		local_node->api_context->conns[i] = malloc (sizeof (struct s_conns));
		if (!local_node->api_context->conns[i]) {
			libp2p_logger_error("api", "Fail to allocate memory to accept connection.\n");
			pthread_mutex_unlock(&local_node->api_context->conns_lock);
			libp2p_logger_debug("api", "api_listen_thread: memory failure, unlock successful.\n");
			close (s);
			continue;
		}
		if (inet_ntop(AF_INET, &ipv4, client, INET_ADDRSTRLEN) == NULL)
			strcpy(client, "UNKNOW");
		local_node->api_context->conns[i]->socket = s;
		local_node->api_context->conns[i]->ipv4   = ipv4;
		local_node->api_context->conns[i]->port   = port;
		// create a struct, which the thread is responsible to destroy
		struct ApiConnectionParam* connection_param = (struct ApiConnectionParam*) malloc(sizeof(struct ApiConnectionParam));
		if (connection_param == NULL) {
			libp2p_logger_error("api", "api_listen_thread: Unable to allocate memory.\n");
			pthread_mutex_unlock(&local_node->api_context->conns_lock);
			libp2p_logger_debug("api", "api_listen_thread: memory failure 2 unlock successful.\n");
			close (s);
			continue;
		}
		connection_param->index = i;
		connection_param->this_node = local_node;
		if (pthread_create(&(local_node->api_context->conns[i]->pthread), NULL, api_connection_thread, (void*)connection_param)) {
			libp2p_logger_error("api", "Create pthread fail.\n");
			free (local_node->api_context->conns[i]);
			local_node->api_context->conns[i] = NULL;
			local_node->api_context->conns_count--;
			close(s);
		} else {
			local_node->api_context->conns_count++;
		}
		libp2p_logger_debug("api", "API for %s: Accept connection %s:%d (%d/%d), pthread %d.\n", local_node->identity->peer->id, client, port, local_node->api_context->conns_count, local_node->api_context->max_conns, i+1);
		pthread_mutex_unlock(&local_node->api_context->conns_lock);
		libp2p_logger_debug("api", "api_listen_thread:Api connection cleanup unlock successful.\n");
	}
	api_connections_cleanup (local_node);
	return NULL;
}

struct ApiContext* api_context_new() {
	struct ApiContext* context = (struct ApiContext*) malloc(sizeof(struct ApiContext));
	if (context != NULL) {
		context->conns = NULL;
		context->conns_count = 0;
		context->ipv4 = 0;
		context->max_conns = 0;
		context->port = 0;
		context->socket = 0;
		context->timeout = 0;
		pthread_mutex_init(&context->conns_lock, NULL);
	}
	return context;
}

/**
 * Start API interface daemon.
 * @param local_node the context
 * @param max_conns.
 * @param timeout time out of client connection.
 * @returns 0 when failure or 1 if success.
 */
int api_start (struct IpfsNode* local_node, int max_conns, int timeout)
{
	int s;
	size_t alloc_size = sizeof(void*) * max_conns;

	struct MultiAddress* my_address = multiaddress_new_from_string(local_node->repo->config->addresses->api);

	char* ip = NULL;
	multiaddress_get_ip_address(my_address, &ip);
	int port = multiaddress_get_ip_port(my_address);

	local_node->api_context = api_context_new();
	if (local_node->api_context == NULL) {
		multiaddress_free(my_address);
		return 0;
	}

	local_node->api_context->ipv4 = hostname_to_ip(ip); // api is listening only on loopback.
	local_node->api_context->port = port;

	if ((s = socket_listen(socket_tcp4(), &(local_node->api_context->ipv4), &(local_node->api_context->port))) <= 0) {
		libp2p_logger_error("api", "Failed to init API. port: %d\n", port);
		return 0;
	}

	local_node->api_context->socket = s;
	local_node->api_context->max_conns = max_conns;
	local_node->api_context->timeout = timeout;

	local_node->api_context->conns = malloc (alloc_size);
	if (!local_node->api_context->conns) {
		close (s);
		libp2p_logger_error("api", "Error allocating memory.\n");
		return 0;
	}
	memset(local_node->api_context->conns, 0, alloc_size);

	if (pthread_create(&local_node->api_context->api_thread, NULL, api_listen_thread, (void*)local_node)) {
		close (s);
		free (local_node->api_context->conns);
		local_node->api_context->conns = NULL;
		local_node->api_context->api_thread = 0;
		libp2p_logger_error("api", "Error creating thread for API.\n");
		return 0;
	}

	libp2p_logger_debug("api", "Started API on localhost port %d.\n", port);
	return 1;
}

/**
 * Stop API.
 * @returns 0 when failure or 1 if success.
 */
int api_stop (struct IpfsNode *local_node)
{
	if (local_node->api_context->api_thread == 0) return 0;
	pthread_cancel(local_node->api_context->api_thread);

	api_connections_cleanup (local_node);

	local_node->api_context->api_thread = 0;

	return 1;
}
