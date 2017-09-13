#include "ipfs/core/api.h"
#include "libp2p/utils/logger.h"

int test_core_api_startup_shutdown() {
	if (!api_start(1234, 10, 5)) {
		libp2p_logger_error("test_api", "api_start failed.\n");
		return 0;
	}
	sleep(5);
	if (!api_stop()) {
		libp2p_logger_error("test_api", "api_stop failed.\n");
		return 0;
	}
	return 1;
}
