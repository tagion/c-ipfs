#ifndef NAMESYS_H
    #define NAMESYS_H

    #define DefaultDepthLimit 32

    #ifdef NAMESYS_C
        char *ErrNamesys[] = {
            NULL,
            "ErrAllocFailed",
            "ErrNULLPointer",
            "ErrPipe",
            "ErrPoll",
            "Could not publish name."
            "Could not resolve name.",
            "Could not resolve name (recursion limit exceeded).",
            "expired record",
            "unrecognized validity type",
            "not a valid proquint string",
            "not a valid domain name",
            "not a valid dnslink entry"
        };
    #else
        extern char *ErrNamesys;
    #endif // NAMESYS_C

    enum {
        ErrAllocFailed = 1,
        ErrNULLPointer,
        ErrPipe,
        ErrPoll,
        ErrPublishFailed,
        ErrResolveFailed,
        ErrResolveRecursion,
        ErrExpiredRecord,
        ErrUnrecognizedValidity,
        ErrInvalidProquint,
        ErrInvalidDomain,
        ErrInvalidDNSLink
    } NamesysErrs;

    typedef struct s_resolvers {
        char *protocol;
        int (*func)(char**, char*);
    } resolvers;

    // Resolver provides path resolution to IPFS
    // It has a pointer to a DAGService, which is uses to resolve nodes.
    // TODO: now that this is more modular, try to unify this code with the
    //       the resolvers in namesys
    typedef struct s_resolver {
        //DAGService DAG;
        //NodeLink *lnk;
        // resolveOnce looks up a name once (without recursion).
        int (*resolveOnce) (char **, char *);
    } resolver;

    //TODO ciPrivKey from c-libp2p-crypto
    typedef void* ciPrivKey;

    typedef struct s_publishers {
        char *protocol;
        int (*func) (ciPrivKey, char*);
        int (*func_eol) (ciPrivKey, char*, time_t);
    } publishers;

    typedef struct s_mpns {
        resolvers  *resolver;
        publishers *publisher;
    } mpns;

    typedef struct s_tlds {
        char *str;
        int  condition;
    } tlds;

    int ipfs_namesys_resolve (resolver *r, char **p, char *str, int depth, char **prefixes);
    int ipfs_namesys_resolve(char **path, char *name);
    int ipfs_namesys_resolve_n(char **path, char *name, int depth);
    int ipfs_namesys_resolve_once (char **path, char *name);
    int ipfs_namesys_publish (char *proto, ciPrivKey name, char *value);
    int ipfs_namesys_publish_with_eol (char *proto, ciPrivKey name, char *value, time_t eol);

    int ipfs_proquint_is_proquint(char *str);
    char *ipfs_proquint_encode(char *buf, int size);
    char *ipfs_proquint_decode(char *str);
    int ipfs_proquint_resolve_once (char **p, char *name);

    int ipfs_isdomain_match_string (char *d);
    int ipfs_isdomain_is_icann_tld(char *s);
    int ipfs_isdomain_is_extended_tld (char *s);
    int ipfs_isdomain_is_tld (char *s);
    int ipfs_isdomain_is_domain (char *s);

    typedef struct s_DNSResolver {
        // TODO
    } DNSResolver;

    int ipfs_dns_resolver_resolve_once (DNSResolver *r, char **path, char *name);
    int ipfs_dns_work_domain (int output, DNSResolver *r, char *name);
    int ipfs_dns_parse_entry (char **Path, char *txt);
    int ipfs_dns_try_parse_dns_link (char **Path, char *txt);
#endif //NAMESYS_H
