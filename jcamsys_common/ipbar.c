// ipbar.c
// Keep a list of IP addresses that failed to authenticate

// gcc ipbar.c -DTEST
// TODO:   	test ipbar_find_olest()


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "ipbar.h"


void ipbar_init(struct ipbarlist* iplist)
{
	bzero(iplist,sizeof(struct ipbarlist[IPBAR_TABLE_LEN]));
}


int ipbar_find_olest(struct ipbarlist* iplist)
{
	time_t  toldest;
	//time_t	tnow;
	//int idx=0;
	int i=0;

	//time(&tnow);
	toldest = iplist[0].lastfailtime;
  	for (i=0; i<IPBAR_TABLE_LEN; i++)
   	{
		if (toldest < iplist[i].lastfailtime)
		{
       			toldest = iplist[i].lastfailtime;
       			//idx = i;
		}
	}
	return(toldest);
}


int ipbar_find_free(struct ipbarlist* iplist)
{
	int i=0;
	//time_t	tnow;

	for (i=0;i<IPBAR_TABLE_LEN;i++)
	{
		if (iplist[i].ip==0)		// free slot
			return(i);
	}
	return(ipbar_find_olest(iplist));
}


// Return number of fails for this ip
int ipbar_failed_connections(struct ipbarlist* iplist,uint32_t ip)
{
	int i=0;

	for (i=0;i<IPBAR_TABLE_LEN;i++)
	{
		if (iplist[i].ip==ip)		// found in table
			return(iplist[i].failattempts);
	}
	return(-1);				// not found
}


// Find ip in table or return next free
int ipbar_find_ip(struct ipbarlist* iplist,uint32_t ip)
{
	int i=0;

	for (i=0;i<IPBAR_TABLE_LEN;i++)
	{
		if (iplist[i].ip==ip)
			return(i);
	}
	return(ipbar_find_free(iplist));
}


// remove ip and fail counter
void ipbar_clear_entry(struct ipbarlist* iplist,uint32_t ip)
{
	int idx=0;

	idx=ipbar_find_ip(iplist,ip);		// ip or free table slot
	iplist[idx].ip=0;			// free up the slot 
	iplist[idx].lastfailtime=0;
	iplist[idx].failattempts=0;
}


// Add a fail attempt, returns the number of fails for that ip to date
int ipbar_add_fail(struct ipbarlist* iplist,uint32_t ip)
{
	int idx=0;

	idx=ipbar_find_ip(iplist,ip);		// ip or free table slot
	iplist[idx].ip=ip;			// place ip in table
	time(&iplist[idx].lastfailtime);	// time in table
	iplist[idx].failattempts++;		// record the fail
	return(iplist[idx].failattempts);
}


#ifdef TEST
int main()
{
	struct ipbarlist iplist[IPBAR_TABLE_LEN];
}
#endif
