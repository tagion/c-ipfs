#include <stdio.h>
#include <string.h>

#include "ipfs/repo/init.h"
#include "ipfs/importer/importer.h"
#include "ipfs/importer/exporter.h"

void stripit(int argc, char** argv) {
	char tmp[strlen(argv[argc])];
	strcpy(tmp, &argv[argc][1]);
	tmp[strlen(tmp)-1] = 0;
	strcpy(argv[argc], tmp);
	return;
}

void strip_quotes(int argc, char** argv) {
	for(int i = 0; i < argc; i++) {
		if (argv[i][0] == '\'' && argv[i][strlen(argv[i])-1] == '\'') {
			stripit(i, argv);
		}
	}
}

#define INIT 1
#define ADD 2
#define OBJECT_GET 3

/***
 * Basic parsing of command line arguments to figure out where the user wants to go
 */
int parse_arguments(int argc, char** argv) {
	if (argc == 1) {
		printf("No parameters passed.\n");
		return 0;
	}
	if (strcmp("init", argv[1]) == 0) {
		return INIT;
	}
	if (strcmp("add", argv[1]) == 0) {
		return ADD;
	}
	if (strcmp("object", argv[1]) == 0 && argc > 2 && strcmp("get", argv[2]) == 0) {
		return OBJECT_GET;
	}
	return -1;
}

/***
 * The beginning
 */
int main(int argc, char** argv) {
	strip_quotes(argc, argv);
	int retVal = parse_arguments(argc, argv);
	switch (retVal) {
	case (INIT):
		return ipfs_repo_init(argc, argv);
		break;
	case (ADD):
		ipfs_import(argc, argv);
		break;
	case (OBJECT_GET):
		ipfs_exporter_object_get(argc, argv);
		break;
	}
}
