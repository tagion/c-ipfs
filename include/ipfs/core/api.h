#pragma once

#ifdef __x86_64__
	#define INT_TYPE uint64_t
#else
	#define INT_TYPE uint32_t
#endif

#define MAX_READ (32*1024) // 32k

struct s_list {
	int socket;
	uint32_t ipv4;
	uint16_t port;
	int max_conns;
	int timeout;
	struct s_conns {
		int socket;
		uint32_t ipv4;
		uint16_t port;
		pthread_t pthread;
	} **conns;
};

#define write_cstr(f,s)	write(f,s,sizeof(s)-1)
#define write_str(f,s)	write(f,s,strlen(s))

#define cstrstart(a,b) (memcmp(a,b,sizeof(b)-1)==0)
#define strstart(a,b) (memcmp(a,b,strlen(b))==0)

void *api_connection_thread (void *ptr);
void api_connections_cleanup (void);
void *api_listen_thread (void *ptr);
int api_start (uint16_t port, int max_conns, int timeout);
int api_stop (void);
