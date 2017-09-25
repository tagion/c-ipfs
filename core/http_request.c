#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "libp2p/utils/vector.h"
#include "libp2p/utils/logger.h"
#include "ipfs/core/http_request.h"
#include "ipfs/namesys/resolver.h"
#include "ipfs/namesys/publisher.h"

/**
 * Handles HttpRequest and HttpParam
 */

/***
 * Build a new HttpRequest
 * @returns the newly allocated HttpRequest struct
 */
struct HttpRequest* ipfs_core_http_request_new() {
	struct HttpRequest* result = (struct HttpRequest*) malloc(sizeof(struct HttpRequest));
	if (result != NULL) {
		result->command = NULL;
		result->sub_command = NULL;
		result->params = libp2p_utils_vector_new(1);
		result->arguments = libp2p_utils_vector_new(1);
	}
	return result;
}

/***
 * Clean up resources of a HttpRequest struct
 * @param request the struct to destroy
 */
void ipfs_core_http_request_free(struct HttpRequest* request) {
	if (request != NULL) {
		//if (request->command != NULL)
		//	free(request->command);
		//if (request->sub_command != NULL)
		//	free(request->sub_command);
		if (request->params != NULL) {
			for(int i = 0; i < request->params->total; i++) {
				struct HttpParam* curr_param = (struct HttpParam*) libp2p_utils_vector_get(request->params, i);
				ipfs_core_http_param_free(curr_param);
			}
			libp2p_utils_vector_free(request->params);
		}
		if (request->arguments != NULL) {
			// arguments should not be dynamically allocated
			//for(int i = 0; i < request->arguments->total; i++) {
			//	free((char*)libp2p_utils_vector_get(request->arguments, i));
			//}
			libp2p_utils_vector_free(request->arguments);
		}
		free(request);
	}
}

/***
 * Build a new HttpParam
 * @returns a newly allocated HttpParam struct
 */
struct HttpParam* ipfs_core_http_param_new() {
	struct HttpParam* param = (struct HttpParam*) malloc(sizeof(struct HttpParam));
	if (param != NULL) {
		param->name = NULL;
		param->value = NULL;
	}
	return param;
}

/***
 * Clean up resources allocated by a HttpParam struct
 * @param param the struct to destroy
 */
void ipfs_core_http_param_free(struct HttpParam* param) {
	if (param != NULL) {
		if (param->name != NULL)
			free(param->name);
		if (param->value != NULL)
			free(param->value);
		free(param);
	}
}

/***
 * Handle processing of the "name" command
 * @param local_node the context
 * @param request the incoming request
 * @param response the response
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_core_http_process_name(struct IpfsNode* local_node, struct HttpRequest* request, char** response) {
	int retVal = 0;
	char* path = NULL;
	char* result = NULL;
	if (request->arguments == NULL || request->arguments->total == 0) {
		libp2p_logger_error("http_request", "process_name: empty arguments\n");
		return 0;
	}
	path = (char*) libp2p_utils_vector_get(request->arguments, 0);
	if (strcmp(request->sub_command, "resolve") == 0) {
		retVal = ipfs_namesys_resolver_resolve(local_node, path, 1, &result);
		if (retVal) {
			*response = (char*) malloc(strlen(result) + 20);
			sprintf(*response, "{ \"Path\": \"%s\" }", result);
		}
		free(result);
	} else if (strcmp(request->sub_command, "publish") == 0) {
		retVal = ipfs_namesys_publisher_publish(local_node, path);
		if (retVal) {
			*response = (char*) malloc(strlen(local_node->identity->peer->id) + strlen(path) + 30);
			sprintf(*response, "{ \"Name\": \"%s\"\n \"Value\": \"%s\" }", local_node->identity->peer->id, path);
		}
	}
	return retVal;
}

/***
 * Handle processing of the "object" command
 * @param local_node the context
 * @param request the incoming request
 * @param response the response
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_core_http_process_object(struct IpfsNode* local_node, struct HttpRequest* request, char** response) {
	int retVal = 0;
	if (strcmp(request->sub_command, "get") == 0) {
		// do an object_get
	}
	return retVal;
}

/***
 * Process the parameters passed in from an http request
 * @param local_node the context
 * @param request the request
 * @param response the response
 * @returns true(1) on success, false(0) otherwise.
 */
int ipfs_core_http_request_process(struct IpfsNode* local_node, struct HttpRequest* request, char** response) {
	if (request == NULL)
		return 0;
	int retVal = 0;

	if (strcmp(request->command, "name") == 0) {
		retVal = ipfs_core_http_process_name(local_node, request, response);
	} else if (strcmp(request->command, "object") == 0) {
		retVal = ipfs_core_http_process_object(local_node, request, response);
	}
	return retVal;
}

/**
 * just builds the basics of an http api url
 * @param local_node where to get the ip and port
 * @returns a string in the form of "http://127.0.0.1:5001/api/v0" (or whatever was in the config file for the API)
 */
char* ipfs_core_http_request_build_url_start(struct IpfsNode* local_node) {
	char* host = NULL;
	char port[10] = "";
	struct MultiAddress* ma = multiaddress_new_from_string(local_node->repo->config->addresses->api);
	if (ma == NULL)
		return NULL;
	if (!multiaddress_get_ip_address(ma, &host)) {
		multiaddress_free(ma);
		return 0;
	}
	int portInt = multiaddress_get_ip_port(ma);
	sprintf(port, "%d", portInt);
	int len = 18 + strlen(host) + strlen(port);
	char* retVal = malloc(len);
	sprintf(retVal, "http://%s:%s/api/v0", host, port);
	free(host);
	multiaddress_free(ma);
	return retVal;
}

/***
 * Adds commands to url
 * @param request the request
 * @param url the starting url, and will hold the results (i.e. http://127.0.0.1:5001/api/v0/<command>/<sub_command>
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_core_http_request_add_commands(struct HttpRequest* request, char** url) {
	// command
	int addl_length = strlen(request->command) + 2;
	char* string1 = (char*) malloc(strlen(*url) + addl_length);
	sprintf(string1, "%s/%s", *url, request->command);
	free(*url);
	*url = string1;
	// sub_command
	if (request->sub_command != NULL) {
		addl_length = strlen(request->sub_command) + 2;
		string1 = (char*) malloc(strlen(*url) + addl_length);
		sprintf(string1, "%s/%s", *url, request->sub_command);
		free(*url);
		*url = string1;
	}
	return 1;
}

/***
 * Add parameters and arguments to the URL
 * @param request the request
 * @param url the url to continue to build
 */
int ipfs_core_http_request_add_parameters(struct HttpRequest* request, char** url) {
	// params
	if (request->params != NULL) {
		for (int i = 0; i < request->params->total; i++) {
			struct HttpParam* curr_param = (struct HttpParam*) libp2p_utils_vector_get(request->params, i);
			int len = strlen(curr_param->name) + strlen(curr_param->value) + 2;
			char* str = (char*) malloc(strlen(*url) + len);
			if (str == NULL) {
				return 0;
			}
			sprintf(str, "%s/%s=%s", *url, curr_param->name, curr_param->value);
			free(*url);
			*url = str;
		}
	}
	// args
	if (request->arguments != NULL) {
		int isFirst = 1;
		for(int i = 0; i < request->arguments->total; i++) {
			char* arg = (char*) libp2p_utils_vector_get(request->arguments, i);
			int len = strlen(arg) + strlen(*url) + 6;
			char* str = (char*) malloc(len);
			if (str == NULL)
				return 0;
			if (isFirst)
				sprintf(str, "%s?arg=%s", *url, arg);
			else
				sprintf(str, "%s&arg=%s", *url, arg);
			free(*url);
			*url = str;
		}
	}
	return 1;
}

struct curl_string {
	char* ptr;
	size_t len;
};

size_t curl_cb(void* ptr, size_t size, size_t nmemb, struct curl_string* str) {
	size_t new_len = str->len + size * nmemb;
	str->ptr = realloc(str->ptr, new_len + 1);
	if (str->ptr != NULL) {
		memcpy(str->ptr + str->len, ptr, size*nmemb);
		str->ptr[new_len] = '\0';
		str->len = new_len;
	}
	libp2p_logger_debug("http_request", "curl_cb received %s.\n", str->ptr);
	return size + nmemb;
}

/**
 * Do an HTTP Get to the local API
 * @param local_node the context
 * @param request the request
 * @param result the results
 * @returns true(1) on success, false(0) on error
 */
int ipfs_core_http_request_get(struct IpfsNode* local_node, struct HttpRequest* request, char** result) {
	if (request == NULL || request->command == NULL)
		return 0;

	char* url = ipfs_core_http_request_build_url_start(local_node);
	if (url == NULL)
		return 0;

	if (!ipfs_core_http_request_add_commands(request, &url)) {
		free(url);
		return 0;
	}

	if (!ipfs_core_http_request_add_parameters(request, &url)) {
		free(url);
		return 0;
	}

	// do the GET using libcurl
	CURL *curl;
	CURLcode res;
	struct curl_string s;
	s.len = 0;
	s.ptr = malloc(1);
	s.ptr[0] = '\0';

	curl = curl_easy_init();
	if (!curl) {
		return 0;
	}
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res == CURLE_OK) {
		*result = s.ptr;
	} else {
		libp2p_logger_error("http_request", "Results of [%s] returned failure. Return value: %d.\n", url, res);
	}
	return res == CURLE_OK;
}

