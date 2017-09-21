#include <stdlib.h>
#include "ipfs/cmd/cli.h"

int cli_get_verb_index(struct CliArguments* args) {
	return 0;
}

char* cli_get_config_dir(struct CliArguments* args) {
	return NULL;
}


struct CliArguments* cli_arguments_new(int argc, char** argv) {
	struct CliArguments* args = (struct CliArguments*) malloc(sizeof(struct CliArguments));
	if (args != NULL) {
		args->argc = argc;
		args->argv = argv;
		args->verb_index = cli_get_verb_index(args);
		args->config_dir = cli_get_config_dir(args);
	}
	return args;
}

void cli_arguments_free(struct CliArguments* args) {
	free(args);
}
