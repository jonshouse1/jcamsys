// jcamsys_sharedmem.c
// 95% effective, not quite good enough

// Should be 'atomic' safe locks, doesn't quite work at the moment

// see:
//http://www.csc.villanova.edu/~mdamian/threads/posixsem2.html
//http://man7.org/linux/man-pages/man2/futex.2.html
//https://www.ibm.com/developerworks/library/l-linux-synchronization/index.html
//http://logan.tw/posts/2018/01/07/posix-shared-memory/
//https://gcc.gnu.org/onlinedocs/gcc-4.7.3/gcc/_005f_005fatomic-Builtins.html



// server: lock was cleared alread
// client: decode_jpeg() ends: 6B DD exp:FF D9, not a jpeg
// sometimes client segfault in 


//#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "jcamsys_server_settings.h"
#include "ipbar.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"


#include <stdatomic.h>

// All these are write locks, grab a lock, do a write, clear a lock
// caller passes in its PID as a unique identifier

void jc_sm_clearlocks(struct sharedmem* sm)
{
	int x=0;
	int i=0;

return;
        for (x=0;x<JC_MAX_IMAGES;x++)
        {
                for (i=0;i<JC_MAX_CAMS;i++)
                        sm->cdata_wlock[x][i]=0; 
        }
}





void jc_sm_release_cdata_lock(struct sharedmem* sm, atomic_int p, int img, int cam)
{
return;
	if (sm->cdata_wlock[img][cam]==0)			// lock isnt on
	{
		printf("lock was cleared already\n"); fflush(stdout);
		return;
	}

	if (sm->cdata_wlock[img][cam]==p)			// my lock ?
	{
		sm->cdata_wlock[img][cam]=0;			// then just clear it
		return;
	}

	sm->cdata_wlock[img][cam]=0; 				// clear it
	printf("wasnt my lock\n"); 
	fflush(stdout);
	// wait a little and see if someone else unlocks it
}




void jc_sm_get_cdata_lock(struct sharedmem* sm, atomic_int p, int img, int cam)
{
return;
	int x=0;

	for (x=0;x<100;x++)					// 10ms worth
	{
		if (sm->cdata_wlock[img][cam]==0)		// if lock is not owned
		{
			if (sm->cdata_wlock[img][cam]==0)	// still not owned
			{
				sm->cdata_wlock[img][cam]=p;	// grab it
				sm->cdata_wlock[img][cam]=p;
				sm->cdata_wlock[img][cam]=p;
			}
			return;
		}
		usleep(100);
	}
	sm->cdata_wlock[img][cam]=p;			
	printf("get locktimeout mypid=%d lockpid=%d\n",p,sm->cdata_wlock[img][cam]); 
	fflush(stdout);
}

