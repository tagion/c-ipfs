#include <stdlib.h>
#include <string.h>

#define IPFS_PIN_C
#include "ipfs/pin/pin.h"

#include "ipfs/cid/cid.h"
#include "ipfs/datastore/key.h"
#include "ipfs/util/errs.h"

// package pin implements structures and methods to keep track of
// which objects a user wants to keep stored locally.

#define PIN_DATASTOREKEY_SIZE 100
char *pinDatastoreKey = NULL;
size_t pinDatastoreKeySize = 0;

struct Cid *emptyKey = NULL;

int ipfs_pin_init ()
{
    int err;
    unsigned char *empty_hash = (unsigned char*) "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zR1n";

    if (!pinDatastoreKey) { // initialize just one time.
        pinDatastoreKey = malloc(PIN_DATASTOREKEY_SIZE);
        if (!pinDatastoreKey) {
            return ErrAllocFailed;
        }
        err = ipfs_datastore_key_new("/local/pins", pinDatastoreKey, PIN_DATASTOREKEY_SIZE, &pinDatastoreKeySize);
        if (err) {
            free (pinDatastoreKey);
            pinDatastoreKey = NULL;
            return err;
        }

        if (!ipfs_cid_protobuf_decode(empty_hash, strlen ((char*)empty_hash), &emptyKey)) {
            return ErrCidDecodeFailed;
        }
    }

    return 0;
}
