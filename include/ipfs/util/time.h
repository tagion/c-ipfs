#ifndef IPFS_TIME_H
    #define IPFS_TIME_H

    struct stime {
        time_t t;
        struct timespec ts;
    };

    int get_gmttime(struct stime *st);
    int ipfs_util_time_parse_RFC3339 (struct stime *st, char *s);
    char *ipfs_util_time_format_RFC3339 (struct stime *st);
#endif // IPFS_TIME_H
