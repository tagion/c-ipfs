#ifndef IPFS_TIME_H
    #define IPFS_TIME_H

    int ipfs_util_time_parse_RFC3339 (struct timespec *ts, char *s);
    char *ipfs_util_time_format_RFC3339 (struct timespec *ts);
#endif // IPFS_TIME_H
