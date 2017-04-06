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

/**
 * Get the correct repo directory. Looks in all the appropriate places
 * for the ipfs directory.
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @param repo_dir the results. This will point to the [IPFS_PATH]/.ipfs directory
 * @returns true(1) if the directory is there, false(0) if it is not.
 */
int ipfs_repo_get_directory(int argc, char** argv, char** repo_dir);

