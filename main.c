#include "main.h" 

/*	ssl support - development...

	#include <openssl/crypto.h>
	#include <openssl/x509.h>
	#include <openssl/pem.h>
	#include <openssl/ssl.h>
	#include <openssl/err.h>

	#define CHK_NULL(x) if ((x)==NULL) exit (1)
	#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
	#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

*/

#define MAX_EVENTS 4096
#define BACKLOG 1024
#define PORT 8088
#define CRLF "\r\n"
#define DOUBLE_CRLF CRLF CRLF

#ifndef SOCKET_NON_BLOCKING
#define SOCKET_NON_BLOCKING(fd)           \
    int flags = fcntl(fd, F_GETFL, 0);    \
    if(flags == -1) {                     \
        perror("fcntl"); abort(); }       \
    flags |= O_NONBLOCK;                  \
    if(fcntl(fd, F_SETFL, flags) == -1) { \
		perror("fcntl"); abort(); }
#endif

#define EPOLL_CTL(efd, a, cfd, evs)          \
    if (epoll_ctl(efd, a, cfd,               \
		&(struct epoll_event){.events = evs, \
		.data = {.fd = cfd}}) == -1) {       \
        perror("epoll_ctl");                 \
        exit(EXIT_FAILURE);                  \
    }

const char *content = "HTTP/1.1 200 OK" CRLF 
"Content-Length: 18" CRLF "Content-Type: text/html" 
DOUBLE_CRLF "Users Static Files" DOUBLE_CRLF;

void do_use_fd(int epollfd, struct epoll_event *d) {
	if (d->events & EPOLLIN) {
		char buf[512] = {0};
		int n = read(d->data.fd, buf, 512);
		if (n > 0) {
			EPOLL_CTL(epollfd, EPOLL_CTL_MOD, d->data.fd, EPOLLOUT);				
		} else {
			if (n == 0) {
				EPOLL_CTL(epollfd, EPOLL_CTL_DEL, d->data.fd, 0);
				close(d->data.fd);
				return;				
			}		
		}
	} else if (d->events & EPOLLOUT) {
		int n = write(d->data.fd, content, 86);
		if (n > 0) {
			EPOLL_CTL(epollfd, EPOLL_CTL_MOD, d->data.fd, EPOLLIN);
		} else {
			if (n == 0) {
				EPOLL_CTL(epollfd, EPOLL_CTL_DEL, d->data.fd, 0);
				close(d->data.fd);
			}
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				printf("close EPOLLOUT %d\n", errno);
				close(d->data.fd);
			}
		}
	} else {
		printf("fd & events: %d %d\n", d->data.fd, d->events);
		EPOLL_CTL(epollfd, EPOLL_CTL_DEL, d->data.fd, 0);
		close(d->data.fd);
	}
	return;
}

void sig_handler(int num) {
	printf("signal: %d\n", num);
  //free mem
	exit(EXIT_FAILURE);
}

int main(void) { //int argc, char *argv[]

	struct epoll_event ev = {0}, events[MAX_EVENTS];
	struct sockaddr_in server_addr = {0};
	struct linger      linger;
	int    n, listen_sock, nfds, epollfd;

	signal(SIGTERM, sig_handler); // exit program
	signal(SIGSEGV, sig_handler); // memory process error (if error after alloc -> free mem)
	signal(SIGINT, sig_handler);  // interrupt Ctrl + C
	
	START_FLAG_UK;

	listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 
		IPPROTO_TCP); 
	if (listen_sock == -1) { 
		printf("socket creation failed...\n"); 
		exit(EXIT_FAILURE); 
	} else
		printf("Socket successfully created..\n");

	server_addr.sin_family = AF_INET; 
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	server_addr.sin_port = htons(PORT);

	if ((bind(listen_sock, (struct sockaddr*)&server_addr, 
		sizeof(server_addr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(EXIT_FAILURE); 
	} else
		printf("Socket successfully binded..\n"); 

	if ((listen(listen_sock, BACKLOG)) != 0) { 
		printf("Listen failed...\n"); 
		exit(EXIT_FAILURE); 
	} else
		printf("Server listening..\n");

	linger.l_onoff  = 1;
	linger.l_linger = 0;
	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
		(const char *)&linger.l_onoff, sizeof(linger.l_onoff)) == -1) {
		printf("SO_REUSEADDR failed\n"); 
		exit(EXIT_FAILURE); 
	}
	if (setsockopt(listen_sock, SOL_SOCKET, SO_LINGER,
		(const void *)&linger, sizeof(struct linger)) == -1) {
		printf("SO_LINGER failed\n"); 
		exit(EXIT_FAILURE); 
	}
	
	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}
	ev.events = EPOLLIN;
	ev.data.fd = listen_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) 
		== -1) {
		perror("epoll_ctl: listen_sock");
		exit(EXIT_FAILURE);
	}
	for (;;) {
		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == listen_sock) {
				ev.data.fd = accept(listen_sock, NULL, NULL);
				if (ev.data.fd == -1) {
					perror("accept");
					exit(EXIT_FAILURE);
				}
				SOCKET_NON_BLOCKING(ev.data.fd);
                ev.events = EPOLLIN;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd,
					&ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else {
                do_use_fd(epollfd, &events[n]);
            }
        }
    }
}
