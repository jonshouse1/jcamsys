// jcamsys_mkkey 

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>

#include "jcamsys.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_settings.h"

//unsigned char msgbuffer[JC_MSGLEN_MAX];

struct jcamsys_settings         stngs;
struct jcamsys_key              ky;
int keylen=0;




int main(int argc, char **argv)
{
	//char pass[JC_MAX_PASSWORD_LEN];
	int r=0;
	int i=0;
	int fd=-1;
	struct stat st = {0};

	if (getuid!=0)
		printf("UID not 0, you probably need to be root for this to work\n");

	settings_defaults(&stngs);
	if (stat(stngs.etc_path, &st) == -1) 
	{
    		r=mkdir(stngs.etc_path, 0755);
		if (r<0)
		{
			printf("Failed to make %s - need to be root?\n",stngs.etc_path);
			exit(1);
		}
	}

	settings_defaults(&stngs);
	r=jcam_read_keyfile(stngs.filename_shared_key, &ky);
	if (r>JC_MIN_KEYSIZE)
	{
		printf("a valid keyfile '%s' already exists, key length=%d\n",stngs.filename_shared_key,r);
		exit(1);
	}

	r=jcam_generate_random_key(&ky, 0);
	if (r<JC_MIN_KEYSIZE)
		exit(1);
	else	printf("Generated random keyfile, key length=%d\n",r);
	fflush(stdout);

	if (jcam_write_keyfile(stngs.filename_shared_key, &ky)!=JC_OK)
	{
		printf("error writing keyfile - need to be root?\n");
		exit(1);
	}
	printf("Written keyfile %s\n",stngs.filename_shared_key);
	fflush(stdout);

	i=jcam_read_keyfile(stngs.filename_shared_key, &ky);
	if (r!=i)
	{
		printf("Strange fail\n");
	}
	else	
	{
		printf("Ok, written a valid key\n");
		exit(0);
	}
}

