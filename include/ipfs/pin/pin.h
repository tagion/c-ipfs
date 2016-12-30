#ifndef IPFS_PIN_H
    #define IPFS_PIN_H

    #include "ipfs/util/errs.h"

    #ifdef IPFS_PIN_C
        const char *ipfs_pin_linkmap[] = {
            "recursive",
            "direct",
            "indirect",
            "internal",
            "not pinned",
            "any",
            "all"
        };
    #else // IPFS_PIN_C
        extern const char *ipfs_pin_map[];
    #endif // IPFS_PIN_C
    enum {
        Recursive = 0,
        Direct,
        Indirect,
        Internal,
        NotPinned,
        Any,
        All
    };

    typedef int PinMode;

    int ipfs_pin_init ();
#endif // IPFS_PIN_H
