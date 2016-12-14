#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __USE_XOPEN
    #define __USE_XOPEN
#endif // __USE_XOPEN

#ifndef __USE_ISOC11
    #define __USE_ISOC11
#endif // __USE_ISOC11

#include <time.h>
#include "ipfs/util/time.h"

int get_gmttime(struct stime *st) {
    if (!st) {
        return 1;
    }
    if (!timespec_get(&st->ts, TIME_UTC) ||
        !time(&st->t)) {
        return 2;
    }
    return 0;
}

int ipfs_util_time_parse_RFC3339 (struct stime *st, char *s)
{
    char *r;
    struct tm tm;

    if (!st || !s || strlen(s) != 35) {
        return 1;
    }
    r = strptime (s, "%Y-%m-%dT%H:%M:%S", &tm);
    if (!r || *r != '.') {
        return 2;
    }
    st->t = mktime(&tm);
    st->ts.tv_nsec = atoll(++r);
    return 0;
}

char *ipfs_util_time_format_RFC3339 (struct stime *st)
{
    char buf[31], *ret;

    ret = malloc(36);
    if (!ret) {
        return NULL;
    }

    if (strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S.%%09dZ00:00", gmtime(&st->t)) != sizeof(buf)-1 ||
        snprintf(ret, 36, buf, st->ts.tv_nsec) != 35) {
        free (ret);
        return NULL;
    }
    return ret;
}
