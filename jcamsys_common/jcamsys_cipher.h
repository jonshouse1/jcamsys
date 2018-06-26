// Generic discovery protocol for devices or services

struct __attribute__((packed)) jcamsys_key
{
	uint16_t	keylength;	
	char		xorkey[JC_MAX_KEYSIZE];
	char		magic[2];
};




// Prototypes
int jcam_getpass(char *pass, int passsize);
int str2md5(char *str, int length, char*md, int mdlen);
int jcam_write_jcam_passwd(char *filename, char*pass, int passlen);
int jcam_read_pass(int *fd, char*pw, int pwlen);
int jcam_crypt_read_pass(struct jcamsys_key* key,int *fd, char *pw, int pwsize, int pwlen, int timeoutms);
void jcam_generate_random_buffer(char *data, int datalen);
int jcam_generate_random_key(struct jcamsys_key* key, int keylen);
int jcam_write_keyfile(char *filename, struct jcamsys_key* key);
int jcam_read_keyfile(char *filename, struct jcamsys_key* key);
void random_md5(char *md);
int jcam_crypt_write(int* fd, struct jcamsys_key* key, char *data, int datalen);
int jcam_crypt_read(int* fd, struct jcamsys_key* key, char *data, int datalen);
int jcam_locally_obfuscate_password(char *pw, int pwsize, char *obfuscated, int obsize, int len, int trueisenc);
int jcam_getormake_client_obpassword(char* pw, int pwsize);
int jcam_crypt_buf(struct jcamsys_key* key,char *data, int datasize, int datalen);
int jcam_crypt_buf_copy(struct jcamsys_key* key, char* destdata, char *srcdata, int srcdatasize, int datalen);
int jcam_authenticate_with_server(int fd, char *pwe, int pwlen, int timeoutms);
int jcam_read_compare_passphrase(struct jcamsys_key* key,int *fd, char*filename, int timeoutms);
int jcam_read_compare_passphrase_plaintext(int *fd, char*filename);


int jcam_key_exchange(char *kdata, int kdatasize, char *ipaddr, int portno);
