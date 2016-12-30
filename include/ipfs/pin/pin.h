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
    // Return pointer to string or NULL if invalid.
    char *ipfs_pin_mode_to_string (PinMode mode);
    // Return array index or -1 if fail.
    PinMode ipfs_string_to_pin_mode (char *str);
#endif // IPFS_PIN_H
