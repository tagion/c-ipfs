#include <stdlib.h>
#include "libp2p/utils/vector.h"
#include "ipfs/core/http_request.h"

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
		if (request->command != NULL)
			free(request->command);
		if (request->sub_command != NULL)
			free(request->sub_command);
		if (request->params != NULL) {
			for(int i = 0; i < request->params->total; i++) {
				struct HttpParam* curr_param = (struct HttpParam*) libp2p_utils_vector_get(request->params, i);
				ipfs_core_http_param_free(curr_param);
			}
			libp2p_utils_vector_free(request->params);
		}
		if (request->arguments != NULL) {
			for(int i = 0; i < request->arguments->total; i++) {
				free((char*)libp2p_utils_vector_get(request->arguments, i));
			}
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
 * Process the parameters passed in from an http request
 * @param request the request
 * @param response the response
 * @returns true(1) on success, false(0) otherwise.
 */
int ipfs_core_http_request_process(struct HttpRequest* request, char** response) {
	return 0;
}
