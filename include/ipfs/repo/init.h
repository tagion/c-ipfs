#pragma once

/***
 * Makes a new ipfs repository
 * @param path the path to the repository, should end
 * with .ipfs, and the directory should already exist.
 * @returns true(1) on success
 */
int make_ipfs_repository(const char* path);

/**
 * Initialize a repository, called from the command line
 * @param argc number of arguments
 * @param argv arguments
 * @returns true(1) on success
 */
int ipfs_repo_init(int argc, char** argv);
