#include <string.h>
#include "ipfs/namesys/pb.h"

int IpnsEntry_ValidityType_value (char *s)
{
    int r;

    if (!s) {
        return -1; // invalid.
    }

    for (r = 0 ; IpnsEntry_ValidityType_name[r] ; r++) {
        if (strcmp (IpnsEntry_ValidityType_name[r], s) == 0) {
            return r; // found
        }
    }

    return -1; // not found.
}
