/**
 * Helpers for testing
 */

/**
 * Create a new repository in the directory, erasing old one
 * NOTE: base directory must already exist
 */
int drop_and_build_repository(const char* dir);

int drop_build_and_open_repo(const char* path, struct FSRepo** fs_repo);
