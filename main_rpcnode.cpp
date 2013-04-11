#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include "antnode.h"


int main(int argc, char **argv)
{
	int port;

	if(argv[1] == NULL || argv[1] == '\0' || (port = atoi(argv[1])) < 1)
	{
		fprintf(stderr,"Invalid port %s\n", argv[1] == NULL ? "" : argv[1]);
	}

	AntNode node(port);

	node.dispatchLoop();

	// outsocket has to listen for connection
	// simnode has to connect based on the init_node func.

	//loop until termination.

	return 1;
}

