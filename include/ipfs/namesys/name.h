#pragma once

#include "ipfs/cmd/cli.h"

/**
 * We received a cli command "ipfs name". Do the right thing.
 * @param argc number of arguments on the command line
 * @param argv actual command line arguments
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_name(struct CliArguments* args);
