#include <errno.h> 
#include <fcntl.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>

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

#define START_FLAG_UK                            \
	printf(                                        \
		"\x1b[33m"                                   \
			" ____________________________________\n"  \
			"|                                    |\n" \
			"| @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ |\n" \
			"| @################################@ |\n" \
			"| @################################@ |\n" \
			"| @################################@ |\n" \
		"\x1b[0m\x1b[36m"                            \
			"| @################################@ |\n" \
			"| @################################@ |\n" \
			"| @################################@ |\n" \
			"| @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ |\n" \
			"|____________________________________|\n" \
		"\x1b[0m"                                    \
	);	

