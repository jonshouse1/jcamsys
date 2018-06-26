// jcamsys_mkpasswd	- Creates a /etc/jcam_passwd file 
// containing the hash of the password entered.

// gcc -g -O0 -DTEST jcamsys_cipher.c jcamsys_common.c -lcrypto

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
#include "jcamsys_cipher.h"
#include "jcamsys_common.h"
#include "jcamsys_settings.h"


struct jcamsys_settings         stngs;

int main(int argc, char **argv)
{
	int r=0;
	int fd=-1;
	struct stat st = {0};
	char pw[JC_MAX_PASSWORD_LEN];
	char md[JC_MAX_PASSWORD_LEN];
	int pwlen=0;

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

	// Get password as plain text
	printf("password: ");
	pwlen=jcam_getpass((char*)&pw, sizeof(pw));
	if (pwlen<JC_MIN_PASSWORD_LEN)
		printf("password to short\n");

	// write password 
        if (jcam_write_jcam_passwd(stngs.filename_passwd, (char*)&pw, pwlen)!=JC_OK)
		printf("Error writing file %s\n",stngs.filename_passwd);
	bzero(&pw,sizeof(pw));
}





