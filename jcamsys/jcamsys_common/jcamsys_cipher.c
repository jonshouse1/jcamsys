// jcamsys cipher
//	probably just barely 'good enough' rather than secure
//
//	to test:	gcc -g -O0 -DTEST jcamsys_cipher.c jcamsys_common.c -lcrypto
// gcc -g -O0 -DTEST jcamsys_cipher.c md5x.c jcamsys_common.c
//
// 	TODO: buffer some data and do larger read/rites ?


// TODO:                use /etc/machine-id as modified to locally store password
//	Replace len with size in function variables where caller is using sizoef()	


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <openssl/md5.h>
#include "md5x.h"
#include <termios.h>

#include "jcamsys.h"
#include "jcamsys_cipher.h"
#include "jcamsys_common.h"


int jcam_getpass(char *pass, int passsize)
{
    	struct termios oflags, nflags;
	int l=0;

    	/* disabling echo */
    	tcgetattr(fileno(stdin), &oflags);
    	nflags = oflags;
    	nflags.c_lflag &= ~ECHO;
    	nflags.c_lflag |= ECHONL;

    	if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) 
	{
        	perror("tcsetattr");
        	return (JC_ERR);
    	}

    	fgets(pass, passsize, stdin);

    	/* restore terminal */
    	if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) 
	{
        	perror("tcsetattr");
        	return EXIT_FAILURE;
    	}

	l=strlen(pass);
	if (l>0)
	{
    		pass[l - 1] = 0;
		l--;
	}
	return(l);
}



// for 'string' of 'length' generate and md5 hash 'md'
int str2md5(char *str, int length, char*md, int mdlen) 
{
	int n;
	MD5_CTX c;
	unsigned char digest[16];
	char st[2];

 	MD5_Init(&c);
	bzero(md,mdlen);

	while (length > 0) 
    	{
        	if (length > 512) 
            		MD5_Update(&c, str, 512);
		else	MD5_Update(&c, str, length);
        	length -= 512;
        	str += 512;
    	}

    	MD5_Final(digest, &c);
    	for (n = 0; n < 16; ++n) 
	{
		sprintf(st,"%02X", (unsigned int)digest[n]);
		strcat(md,st);	
    	}
	return(strlen(md));
}




// write a file, normally /etc/jcam_passwd containing the md5 hash of a supplied password
int jcam_write_jcam_passwd(char *filename, char*pass, int passlen)
{
	char mdbuf[1024];
	int r=0;
	int s=0;

	bzero(&mdbuf,sizeof(mdbuf));
	s=str2md5((char*)pass,passlen,(char*)&mdbuf,sizeof(mdbuf));
	mdbuf[s]='\n';
	s++;
	r=buffertofile(filename,(char*)&mdbuf, s, 0644);
	if (r==s)
		return(JC_OK);
	return(JC_ERR);
}


// read from file 'fd' into 'pw', return number of bytes read
int jcam_read_pass(int *fd, char *pw, int pwsize)
{
	int idx=0;
	char b=0;
	int r=0;

	bzero(pw,pwsize);
	do
	{
		r=read(*fd,&b,1);								// read from socket 1 byte at a time
		if (r==0)									// socket hungup ?
		{
			close(*fd);
			*fd=-1;
			return(JC_ERR_READ_FAILED);
		}
		if (r==1)									// got the bytes we asked for
		{
			if ( (b!='\r') & (b!='\n') )						// not newline or return
			{
				if (idx<pwsize)							// and not a buffer overflow
					pw[idx++]=b;						// then add it to buffer
				else								// otherwsie it would be an overflow
				{
					close(*fd);						// so close the file
					*fd=-1;
					return(JC_ERR_READ_FAILED);				// and fail
				}
			}
		}
	} while (b!='\n');
	return(idx);										// return length of string
}


// Read password one byte at a time from socket, decrypt it as we go
int jcam_crypt_read_pass(struct jcamsys_key* key,int *fd, char *pw, int pwsize, int pwlen, int timeoutms)
{
	int idx=0;
	unsigned char b=0;
	int r=0;
	int tout=timeoutms;
	unsigned char l=0;

	// Client tells us password length as 8 bit unsigned
	do
	{
		r=read(*fd,&l,1);
//printf("r=%d\t",r);
		if (r==0)
		{
			close(*fd);
			*fd=-1;
			return(JC_ERR_READ_FAILED);
		}
		sleep_ms(1);
		tout=tout-1;
	} while ( (tout>0) & (r!=1) );							// timeout or 1 byte read
//printf("tout=%d  r=%d l=%d(%02X)\n",tout,r,l,l); fflush(stdout);

	if (tout<=0)
		return(JC_ERR_TIMEOUT);							// timeout waiting for 'length'


//printf("jcam_crypt_read_pass() expecting password of length %d\n",l);
	if ( (l<JC_MIN_PASSWORD_LEN) | (l>JC_MAX_PASSWORD_LEN) )
		return(JC_ERR);


	// Read rest of sting one byte at a time xoring with key as we go
	bzero(pw,pwsize);
	do
	{
		r=read(*fd,&b,1);
//printf("r=%d tout=%d idx=%d b=%02X %c\n",r,tout,idx,b,b);  fflush(stdout);
		if (r==0)									// socket hungup ?
		{
			close(*fd);
			*fd=-1;
			return(JC_ERR_READ_FAILED);
		}
		if (r==1)									// got the bytes we asked for
		{
			b=b^key->xorkey [idx % key->keylength];	
			if (idx<pwsize)							// and not a buffer overflow
				pw[idx++]=b;						// then add it to buffer
			else								// otherwsie it would be an overflow
			{
				close(*fd);						// so close the file
				*fd=-1;
				return(JC_ERR_READ_FAILED);				// and fail
			}
		}
		else									// read failed
		{
			sleep_ms(1);
			tout=tout-1;
			if (tout<=0)
				return(JC_ERR_TIMEOUT);
		}
	} while (idx<l); 
	return(idx);										// return length of string
}



// fills a buffer 'data' with datalen bytes of random
void jcam_generate_random_buffer(char *data, int datalen)
{
	int i=0;

	bzero(data,datalen);									// make sure buffer contains only 'our' data
	sleep(1);										// ensure we always get a 'fresh' second
	srand(time(NULL));
	for (i=0;i<datalen;i++)
		data[i]=rand() % 255;
}



// generate a random key,  if keylength is zero the default maximum will be used
int jcam_generate_random_key(struct jcamsys_key* key, int keylen)
{
	int kl=0;

	if (keylen==0)
		kl=JC_MAX_KEYSIZE;
	else	kl=keylen;
	if (kl>JC_MAX_KEYSIZE)
		return(JC_ERR);
	if (kl<JC_MIN_KEYSIZE)
		return(JC_ERR);

	jcam_generate_random_buffer(key->xorkey,kl);
	key->keylength=kl;
	key->magic[0]='J';
	key->magic[1]='J';
	return(kl);
}



// from 'data' of length 'datalen' generate an ASCII key file with HEX in it
int jcam_write_keyfile(char *filename, struct jcamsys_key* key)
{
	int i=0;
	int l=0;
	char dt[1024];
	time_t t = time(NULL);
    	struct tm *tm = localtime(&t);

	strftime(dt, sizeof(dt), "%c", tm);

	FILE *file = fopen (filename,"w");
	if (!file)
		return(JC_ERR);
	fprintf(file,"JCAM Key file, Generated %s\n",dt);
	fprintf(file,"KL=%04d\n",key->keylength);
	for (i=0;i<key->keylength;i++)
	{
		fprintf(file,"%02X",key->xorkey[i]);
		l++;
		if (l==32)
		{
			l=0;
			fprintf(file,"\n");
		}
	}
	fprintf(file,"\n");
	fclose(file);
	return(JC_OK);
}


// Read an ASCII keyfile containing HEX, populate a 'struct jcamsys_key' with its data
// returns key length or error
int jcam_read_keyfile(char *filename, struct jcamsys_key* key)
{
	int kl=0;										// key length
	int gotkl=FALSE;
	int kidx=0;
	int i=0;
	int l=0;
	unsigned int b;
    	char line [1000];
	char ht[8];
  	FILE *file = fopen (filename, "r");

  	if (file != NULL)
	{
    		while(fgets(line,sizeof line,file)!= NULL) 
		{
			if (line[0]=='K' && line[1]=='L' && line[2]=='=')
			{
				sscanf(line,"KL=%04d",&kl);
				if ( (kl<JC_MIN_KEYSIZE) | (kl>JC_MAX_KEYSIZE) )
				{
					printf("KL=%d, must be >=%d and <=%d\n",kl,JC_MIN_KEYSIZE,JC_MAX_KEYSIZE);
					fflush(stdout);
					fclose(file);
					return(JC_ERR);
				}
				gotkl=TRUE;
			}
			else
			{
				if (gotkl==TRUE)
				{
					l=0;
					//bzero(&ht,sizeof(ht));
					for (i=0;i<strlen(line);i++)
					{
						if ( (line[i]>='0') & (line[i]<='F') )
						{
							ht[l++]=line[i];
							if (l==2)
							{
								ht[l]=0;
								l=0;
								sscanf(ht,"%02X",&b);
								key->xorkey[kidx++]=b;
								if (kidx==kl)			// got complete key
								{
									fclose(file);
									key->magic[0]='J';	// marks it as complete
									key->magic[1]='C';
									key->keylength=kl;
									return(kl);
								}
								bzero(&ht,sizeof(ht));
							}
						}
					}
				}	
			}
		}
		fclose(file);
	}
	else	
	{
		//perror(filename);								//print the error message on stderr.
		return(JC_ERR);
	}
	return(JC_ERR);
}


// TODO:
//		Check all possible return results of read() and write() in blocking and non blocking
//		add timeout to jcam_crypt_read() so it does not wait forever 

// Write N bytes of encrypted binary data to a file or socket 'fd'
// normally this would be jpeg data, avoid using it on buffers of zeros as this
// would yeild the key, similarly avoid runs of bytes where possible.
int jcam_crypt_write(int* fd, struct jcamsys_key* key, char *data, int datalen)
{
	int i=0;
	int r=0;
	char b;

	if ( (key->magic[0]!='J') & (key->magic[1]!='C') )					// Check key has been initialised
		return(JC_ERR_BAD_MAGIC);
	if (key->keylength<JC_MIN_KEYSIZE)
		return(JC_ERR_KEY_NOT_VALID);
	do
	{
		b=data[i]^key->xorkey [i % key->keylength];
		r=write(*fd,&b,1);
		if (r==1)									// good write
			i++;
		if (r==0)									// socket closed?
		{
			perror("err");
			close(*fd);
			*fd=-1;
			return(JC_ERR_WRITE_FAILED);						// write failed, file is closed
		}
	} while (i<datalen);
	return(JC_OK);
}



// fill buffer 'data' with 'datalen' bytes of decrypted data read from file or socket 'fd'
int jcam_crypt_read(int* fd, struct jcamsys_key* key, char *data, int datalen)
{
	int i=0;
	int r=0;
	char b;

	if ( (key->magic[0]!='J') & (key->magic[1]!='C') )						// Check key has been initialised
		return(JC_ERR_BAD_MAGIC);
	if (key->keylength<JC_MIN_KEYSIZE)
		return(JC_ERR_KEY_NOT_VALID);
	do
	{
		r=read(*fd,&b,1);
		if (r==1)									// we got our byte
		{
			data[i]=b^key->xorkey [i % key->keylength];
			i++;
		}
		if (r==0)									// socket closed?
		{
			close(*fd);
			*fd=-1;
			return(JC_ERR_READ_FAILED);
		}
	} while (i<datalen);			// Maybe <= ? test it
	return(JC_OK);
}


// Passed a buffer of size 'datasize' encrypt (or decrpt) buffer 
int jcam_crypt_buf(struct jcamsys_key* key, char *data, int datasize, int datalen)
{
	int i=0;
	char b=0;
	if ( (key->magic[0]!='J') & (key->magic[1]!='C') )						// Check key has been initialised
		return(JC_ERR_BAD_MAGIC);
	if (key->keylength<JC_MIN_KEYSIZE)
		return(JC_ERR_KEY_NOT_VALID);
	
	for (i=0;i<datalen;i++)
	{
		b=data[i]^key->xorkey [i % key->keylength];
		data[i]=b;
	}
	return(datalen);
}


// copy a buffer encypting as we go
int jcam_crypt_buf_copy(struct jcamsys_key* key, char* destdata, char *srcdata, int srcdatasize, int datalen)
{
	int i=0;
	//char b=0;
	if ( (key->magic[0]!='J') & (key->magic[1]!='C') )						// Check key has been initialised
		return(JC_ERR_BAD_MAGIC);
	if (key->keylength<JC_MIN_KEYSIZE)
		return(JC_ERR_KEY_NOT_VALID);
	
	for (i=0;i<datalen;i++)
		destdata[i]=srcdata[i]^key->xorkey [i % key->keylength];

	return(datalen);
}



void err_idmissing()
{
	printf("/etc/machine-id missing or bad, please create it\n");
	printf("if all else fails \"md5sum /proc/cpuinfo |cut -d' ' -f1 >/etc/machine-id\" would do\n");
	exit(1);
}

// obfuscate or unobfuscate a password using a unique identifier for the current machine. 
// just xors pw with /etc/machine-id
int jcam_locally_obfuscate_password(char *pw, int pwsize, char *obfuscated, int obsize, int len, int trueisenc)
{
	int i=0;
	//int l=0;
	char machineid[1024];
	int idlen=0;

	bzero(&machineid,sizeof(machineid));
	if (len<JC_MIN_PASSWORD_LEN)
		return(JC_ERR_BAD_PASS);

	idlen=filetobuffer("/etc/machine-id",(char*)&machineid,sizeof(machineid));
	if (idlen>32)
		idlen=32;									// ignore newline or any text after
	if (idlen<=4)
		err_idmissing();

	for (i=0;i<len;i++)
	{
		if (trueisenc==TRUE)
			obfuscated[i]=pw[i]^machineid[i % idlen];
		else	pw[i]=obfuscated[i]^machineid[i % idlen];
	}
	return(len);
}



// Retrieve or make our obfuscated password
int jcam_getormake_client_obpassword(char* pw, int pwsize)
{
	char sbuf1[16384];
	char sbuf2[16384];
	int l=0;
	int i=0;
	bzero(sbuf1,sizeof(sbuf1));
	bzero(sbuf1,sizeof(sbuf2));
	l=filetobuffer(".jcpass",(char*)&sbuf1,sizeof(sbuf1));								// Try current ?HOME directory first 
	if (l<=0)
		l=filetobuffer("/.jcpass",(char*)&sbuf1,sizeof(sbuf1));
	if (l<=0)
		l=filetobuffer("/root/.jcpass",(char*)&sbuf1,sizeof(sbuf1));
	if (l<=0)
		l=filetobuffer("/etc/jcamsys/.jcpass",(char*)&sbuf1,sizeof(sbuf1));
	if (l<=0)
	{
		printf("Missing .jcpass file,  lets create one\n");
		printf("password: ");
		fflush(stdout);
		i=jcam_getpass((char*)&sbuf1, sizeof(sbuf1));
//printf("password len from getpass=%d\n",i);
		if (i>=JC_MIN_PASSWORD_LEN)
		{														// encode file
			l=jcam_locally_obfuscate_password((char*)&sbuf1, sizeof(sbuf1), (char*)&sbuf2, sizeof(sbuf2),i,TRUE);	// encode
			if (l>0)
			{
				l=buffertofile(".jcpass",(char*)&sbuf2,l,0644);					// write file
				memcpy(sbuf1,sbuf2,sizeof(sbuf1));
				if (l==i)
				{
					printf("made .jcpass file\n");
				}
				else	
				{
					printf("Error making .jcpass file ?\n");
					exit(1);
				}
			}
		}
		else
		{
			printf("Bad password, must be >=%d chars\n",JC_MIN_PASSWORD_LEN);
			exit(1);
		}
	}

	i=jcam_locally_obfuscate_password(pw,pwsize,(char*)&sbuf1, sizeof(sbuf1),l,FALSE); // decode
	bzero(&sbuf1,sizeof(sbuf1));
	bzero(&sbuf2,sizeof(sbuf2));
	return(i);														// return pw length
}



// Send authentication to server, parse reply and check all is good, client code
// see jcam_read_compare_passphrase() for the other side of this
// the caller must alredy have used the key on the password
// The protocol is very harsh, no greeting message
int jcam_authenticate_with_server(int fd, char *pw, int pwlen, int timeoutms)
{
        int  r=0;
        int  tout=timeoutms;
        char validresp[4];
        char  inbuf[4];
        int  idx=0;
	unsigned char l=pwlen;						// single byte

//printf("l=%d %02X\n",l,l);
	r=write(fd,&l,1);						// Tell server pwlen
	if (r==-1)
		return(JC_ERR_BAD_AUTH);
//printf("initial write() returned %d\n",r); fflush(stdout);
        r=write(fd,pw,pwlen);						// write password itself
	if (r==-1)
		return(JC_ERR_BAD_AUTH);
//printf("second write returned %d\n",r); fflush(stdout);


	// Look for valid response from server
        bzero(&validresp,sizeof(validresp));
        sprintf(validresp,"AOK");
	bzero(&inbuf,sizeof(inbuf));
	while (memcmp(&inbuf,&validresp,strlen(validresp))!=0)
	{
		r=read(fd,&inbuf[idx],1);
//printf("r=%d\n",r); fflush(stdout);
		if (r==0)						// socket hungup
		{
			if (idx==0)					// did we get any bytes back?
				return(JC_ERR_BAD_AUTH);		// no
			else	return(JC_ERR);				// early hangup
		}
		if (r==1)
		{
			//printf("%c ",inbuf[idx]); fflush(stdout);
			idx++;
		}
		sleep_ms(10);
		tout=tout-10;
		if (tout<=0)
			return(JC_ERR_TIMEOUT);
	}
	return(JC_OK);                                                  // all good
}



// read passphrase from socket, xor with key while reading,  compare its hash to hash from file
// return 0 if valid
// the lack of prompt or ident on connection is by design
// server uses this for authentication, other side of jcam_authenticate_with_server
int jcam_read_compare_passphrase(struct jcamsys_key* key,int *fd, char*filename, int timeoutms)
{
	char buf[16 * 1024];								// file contents
	char mdbuf[1024];								// hashed password
	char pw[JC_MAX_PASSWORD_LEN+1];							// password from user
	int r=0;
	int pwlen=0;									// may be -1 if read fails

	r=filetobuffer(filename,(char*)&buf,sizeof(buf));
	if (r<0)
		return(JC_ERR);
	pwlen=jcam_crypt_read_pass(key,fd,(char*)&pw,sizeof(pw),r,timeoutms);		// read the password from socket
	if (pwlen<=0)
	{										// timeout or disconnect
		bzero(&pw,sizeof(pw));								// blank password
		return(pwlen);				// forward the error from crypt_read_pass
	}
//printf("pwlen=%d\n",pwlen); fflush(stdout);

	r=str2md5((char*)&pw,pwlen,(char*)&mdbuf,sizeof(mdbuf));
	bzero(&pw,sizeof(pw));
//printf("mdbuf=%s\nbuf=%s\n",mdbuf,buf); fflush(stdout);
	if (memcmp(mdbuf,buf,r)==0)								// hashed pw from file = hash from read_pass?
		return(JC_OK);
	return(JC_ERR);
}





int jcam_read_compare_passphrase_plaintext(int *fd, char*filename)
{
	char buf[16 * 1024];								// file contents
	char mdbuf[1024];								// hashed password
	char pw[JC_MAX_PASSWORD_LEN+1];							// password from user
	int r=0;
	int pwlen=0;

	r=filetobuffer(filename,(char*)&buf,sizeof(buf));
	if (r<0)
		return(JC_ERR);
	pwlen=jcam_read_pass(fd,(char*)&pw,sizeof(pw));		// read the password from socket
	if (pwlen<=0)
	{
		bzero(&pw,sizeof(pw));								// blank password
		return(pwlen);				// forward the error from crypt_read_pass
	}

	r=str2md5((char*)&pw,pwlen,(char*)&mdbuf,sizeof(mdbuf));
	bzero(&pw,sizeof(pw));
//printf("mdbuf=%s\nbuf=%s\n",mdbuf,buf); fflush(stdout);
	if (memcmp(mdbuf,buf,r)==0)								// hashed pw from file = hash from read_pass?
		return(JC_OK);
	return(JC_ERR);
}



// under special circumstances a key can be retrieved from server.
// KEY must be >=1024 bytes 
#define JCAMSYS_KEYEXCHANGE_KEY "Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do: once or twice she had peeped into the book her sister was reading, but it had no pictures or conversations in it, and what is the use of a book, thought Alice without pictures or conversations? So she was considering in her own mind (as well as she could, for the hot day made her feel very sleepy and stupid), whether the pleasure of making a daisy-chain would be worth the trouble of getting up and picking the daisies, when suddenly a White Rabbit with pink eyes ran close by her. There was nothing so very remarkable in that; nor did Alice think it so very much out of the way to hear the Rabbit say to itself, ‘Oh dear Oh dear I shall be late’ (when she thought it over afterwards, it occurred to her that she ought to have wondered at this, but at the time it all seemed quite natural); but when the Rabbit actually took a watch out of its waistcoat-pocket, and looked at it, and then hurried on, Alice started to her feet, for it flashed across her mind that she had never before seen a rabbit with either a waistcoat-pocket, or a watch to take out of it, and burning with curiosity, she ran across the field after it, and fortunately was just in time to see it pop down a large rabbit-hole under the hedge." 

int jc_key_exchange()
{
	return 0;
}



#ifdef TEST
int main()
{
	int fd=-1;
	//int i=0;
	int r=0;
	struct jcamsys_key	key;
	char *buf=NULL;
	char sometext[]="THE special trunks now readily procurable for week-end visits remind us not to burden our friends with heavy or excessive luggage. The visitor may have difficulty in deciding what costumes to carry. Hence a considerate hostess often mentions in her note of invitation what the out-of-door amusements are likely to be. If a tennis-court, golf-course, skating-rink, or toboggan-slide is available, she does well to say so. A host who lives by the seaside will perhaps take his guests out in a canoe or a motor-boat or offer them the pleasures of surf-bathing.";

	r=jcam_generate_random_key(&key, 0);
	if (r<=0)
	{
		printf("error jcam_generate_random_key %d\n",r);
		exit(1);
	}
	printf("Generated random key\n");

	r=jcam_write_keyfile("/tmp/testkey", &key);
	if (r!=JC_OK)
	{
		printf("error jcam_write_keyfile %d\n",r);
		exit(1);
	}
	printf("Wrote keyfile /tmp/testkey\n");

	r=jcam_read_keyfile("/tmp/testkey", &key);
	if (r<=0)
	{
		printf("error jcam_read_keyfile %d\n",r);
		exit(1);
	}
	printf("Read keyfile /tmp/testkey, got %d bytes of key data\n",r);


	// write encrypted text into file
	int s=strlen(sometext);
	fd=open("/tmp/somefile",O_WRONLY|O_CREAT, 0666);
	if (fd<0)
	{
		perror("open");
		exit(1);
	}
	r=jcam_crypt_write(&fd,&key,(char*)&sometext,s);
	close(fd);
	if (r<JC_ERR)
	{
		printf("Error %d from jcam_crypt_write\n",r);
		exit(1);
	}
	printf("Wrote encrypted text into /tmp/somefile\n");
	printf("use hexdump -C /tmp/somefile to see file contents\n");


	// decrpt file
	fd=open("/tmp/somefile",O_RDONLY);
	if (fd<0)
	{
		perror("somefile");
		exit(1);
	}
	buf=(char*)malloc(s);
	if (buf==NULL)
	{
		printf("error");
		exit(1);
	}
	bzero(buf,s);
	r=jcam_crypt_read(&fd, &key, (char*)buf, s);	
	close(fd);
	if (r!=JC_OK)
	{
		printf("jcam_crypt_read error %d\n",r);
		exit(1);
	}
	printf("Decrypted /tmp/somefile, displaying here\n");
	printf("\n%s\n",buf);
	free(buf);


	//char *output = str2md5("hello", strlen("hello"));
        //printf("%s\n", output);
	r=str2md5("hello",5,buf,s);
	r=jcam_write_jcam_passwd("/tmp/jcam_passwd", "hello", 5);
        printf("md5 of password 'hello'=%s\n", buf);

	fd=STDIN_FILENO;
	printf("enter password :"); fflush(stdout);
	r=jcam_read_compare_passphrase_plaintext(&fd, "/tmp/jcam_passwd");
	if (r!=JC_OK)
		printf("bad password\n");
	else	printf("good password\n");
	
}
#endif 



