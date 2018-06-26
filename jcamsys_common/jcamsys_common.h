// Prototypes only
void stripnewline(char* st, int len);
void set_realtime();
int volumepercent(char *filename);
int run_command( char* cmd, char *text, int tsize);
void sleep_ms(int milliseconds);
void datetime(void *dt, int flag_short);
void jc_error_text(int jcerror, int addmeaning, char*txt, int textlen);
int loadfile(char *filename, char *buf, int buflen);
int savefile(char *filename, char *buf, int bsize, int mode);
struct tm* GetTimeAndDate(unsigned long long milliseconds);

long long current_timems();
int get_clockseconds();
int hostname_to_ip(char * hostname , char* ip);
int parse_commandlineargs(int argc,char **argv,char *findthis);
int parse_findargument(int argc,char **argv,char *findthis);
int file_exists(char *fname);
pid_t proc_find(const char* name);

void jc_set_time_ms(uint32_t* t);
void DumpHex(const void* data, size_t size);
uint32_t rc_crc32(uint32_t crc, const char *buf, size_t len);
unsigned long current_times();
