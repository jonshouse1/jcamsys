// ipbar.h

#include <stdint.h>
#include <time.h>

#define IPBAR_TABLE_LEN		8000

struct ipbarlist
{
	uint32_t	ip;
	uint16_t	failattempts;
	time_t		lastfailtime;
	int		failcount;
};



// prototypes
void ipbar_init(struct ipbarlist* iplist);
int ipbar_find_olest(struct ipbarlist* iplist);
int ipbar_find_free(struct ipbarlist* iplist);
int ipbar_failed_connections(struct ipbarlist* iplist,uint32_t ip);
int ipbar_find_ip(struct ipbarlist* iplist,uint32_t ip);
void ipbar_clear_entry(struct ipbarlist* iplist,uint32_t ip);
int ipbar_add_fail(struct ipbarlist* iplist,uint32_t ip);
