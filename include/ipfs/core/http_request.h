#pragma once

#include "ipfs/core/ipfs_node.h"

/***
 * A name/value pair of http parameters
 */
struct HttpParam {
	char* name; // the name of the parameter
	char* value; // the value of the parameter
};

/**
 * A struct to help with incoming http requests
 */
struct HttpRequest {
	char* command; // the command
	char* sub_command; // the sub command
	struct Libp2pVector* params; // a collection of HttpParam structs
	struct Libp2pVector* arguments; // a collection of chars that are arguments
};

/***
 * Build a new HttpRequest
 * @returns the newly allocated HttpRequest struct
 */
struct HttpRequest* ipfs_core_http_request_new();

/***
 * Clean up resources of a HttpRequest struct
 * @param request the struct to destroy
 */
void ipfs_core_http_request_free(struct HttpRequest* request);

/***
 * Build a new HttpParam
 * @returns a newly allocated HttpParam struct
 */
struct HttpParam* ipfs_core_http_param_new();

/***
 * Clean up resources allocated by a HttpParam struct
 * @param param the struct to destroy
 */
void ipfs_core_http_param_free(struct HttpParam* param);

/***
 * Process the parameters passed in from an http request
 * @param local_node the context
 * @param request the request
 * @param response the response
 * @returns true(1) on success, false(0) otherwise.
 */
int ipfs_core_http_request_process(struct IpfsNode* local_node, struct HttpRequest* request, char** response);

/**
 * Do an HTTP Get to the local API
 * @param local_node the context
 * @param request the request
 * @param result the results
 * @returns true(1) on success, false(0) on error
 */
int ipfs_core_http_request_get(struct IpfsNode* local_node, struct HttpRequest* request, char** result);
