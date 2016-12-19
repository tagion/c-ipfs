#ifndef IPFS_ERRS_H
    #define IPFS_ERRS_H

    char *Err[] = {
        NULL,
        "ErrAllocFailed",
        "ErrNULLPointer",
        "ErrUnknow",
        "ErrPipe",
        "ErrPoll",
        "Could not publish name."
        "Could not resolve name.",
        "Could not resolve name (recursion limit exceeded).",
        "expired record",
        "unrecognized validity type",
        "not a valid proquint string",
        "not a valid domain name",
        "not a valid dnslink entry",
        // ErrBadPath is returned when a given path is incorrectly formatted
        "invalid 'ipfs ref' path",
        // Paths after a protocol must contain at least one component
        "path must contain at least one component",
        "TODO: ErrCidDecode",
        NULL,
        "no link named %s under %s",
        "ErrInvalidParam",
        // ErrResolveLimit is returned when a recursive resolution goes over
        // the limit.
        "resolve depth exceeded",
        NULL,
        "Invalid value. Not signed by PrivateKey corresponding to %s"
    };

    enum {
        ErrAllocFailed = 1,
        ErrNULLPointer,
        ErrUnknow,
        ErrPipe,
        ErrPoll,
        ErrPublishFailed,
        ErrResolveFailed,
        ErrResolveRecursion,
        ErrExpiredRecord,
        ErrUnrecognizedValidity,
        ErrInvalidProquint,
        ErrInvalidDomain,
        ErrInvalidDNSLink,
        ErrBadPath,
        ErrNoComponents,
        ErrCidDecode,
        ErrNoLink,
        ErrNoLinkFmt,
        ErrInvalidParam,
        ErrResolveLimit,
        ErrInvalidSignature,
        ErrInvalidSignatureFmt
    } ErrsIdx;
#endif // IPFS_ERRS_H
