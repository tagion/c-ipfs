#ifndef IPNS_NAMESYS_PB_H
    #define IPNS_NAMESYS_PB_H

    typedef int IpnsEntry_ValidityType;

    struct ipns_entry {
        // TODO
        struct routingResolver *cache;
        struct stime *eol;
    };

    struct namesys_pb {
        // TODO
        struct ipns_entry *IpnsEntry;
    };

    // setting an EOL says "this record is valid until..."
    const IpnsEntry_ValidityType IpnsEntry_EOL = 0;

    char *IpnsEntry_ValidityType_name[] = {
        "EOL",
        NULL
    };

    int IpnsEntry_ValidityType_value (char *s);
    char* ipfs_namesys_pb_get_validity (struct ipns_entry*);
    char* ipfs_ipns_entry_data_for_sig(struct ipns_entry*);
    char* ipfs_ipns_entry_get_signature(struct ipns_entry*);
    int ipfs_namesys_pb_get_value (char**, struct ipns_entry*);
    IpnsEntry_ValidityType ipfs_namesys_pb_get_validity_type (struct ipns_entry*);
#endif // IPNS_NAMESYS_PB_H
