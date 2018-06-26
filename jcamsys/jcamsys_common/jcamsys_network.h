// jcamsys_network.h

int is_valid_fd(int fd);
int jc_hostname_to_ip(char * hostname , char* ip);
int create_listening_tcp_socket(int port);
int jc_connect_to_server(char *ipaddr, int tcp_port, int blocking);
int jc_fd_blocking(int sockfd,int torf);
int jc_discover_server(char *ipaddr, int *port, int silent);
int jc_peekbytes(int sockfd);

