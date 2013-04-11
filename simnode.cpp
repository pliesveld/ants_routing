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
#include <math.h>

#include <iostream>

#include "simnode.h"


SimNode::SimNode(int port)
{
	struct sockaddr_in my_addr;
	struct sockaddr_in server;
	int reuseOpt = 1;
	int len = sizeof(struct sockaddr_in);

	bzero(&my_addr,sizeof(struct sockaddr_in));
	bzero(&server,sizeof(struct sockaddr_in));

	// Initialize the TCP connections
	listener = socket(PF_INET,SOCK_STREAM,0); //TCP server
	insocket = socket(PF_INET,SOCK_STREAM,0);  //TCP client

	if(listener < 0 || insocket < 0)
		fprintf(stderr,"Invalid socket\n");

	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuseOpt, sizeof(reuseOpt));
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(port);
	my_addr.sin_family = AF_INET;

	if(bind(listener,(struct sockaddr *) &my_addr,sizeof(struct sockaddr_in)) == -1)
	{
		perror("Could not bind to port");
		exit(1);
	}
	listen(listener,5);
	outsocket = accept(listener,(struct sockaddr *)&server,(socklen_t *)&len);

	if(outsocket == -1)
	{
		perror("accept");
		exit(1);
	}

}

SimNode::~SimNode()
{
}

// Outbound functions
int32 SimNode::init_node(int16 _node_id, int16 _nnodes, int16 _nports, 
					int32 _simulator_ip, int16 _simulator_port)
{
	int i,j;
	int now;
	struct sockaddr_in peer;

	node_id = _node_id;
	nnodes = _nnodes;
	nports = _nports;
	simulator_ip = _simulator_ip;
	simulator_port = _simulator_port;

	peer.sin_addr.s_addr = htonl(simulator_ip);
	peer.sin_port = htons(simulator_port);
	peer.sin_family = AF_INET;

	if(connect(insocket,(struct sockaddr *)&peer,sizeof(struct sockaddr_in)) == -1)
	{
		perror("connect");
		exit(1);
	}
	return 0;
}


int32 SimNode::timer_expired(int32 data)
{
	return data;
}




int32 SimNode::read_msg(int16 from_node, int16 port_id, bytearray data)
{
	return 0;
}

int32 SimNode::terminate_node()
{
	printf("SimNode::Terminate node\n");
	return 0;
}


int16 SimNode::get_rtable_port(int16 dest_node)
{
	int ret = -1;
	return ret;
}

int32 SimNode::get_rtable_cost(int16 dest_node)
{
	int lowest = -1;
	return lowest;

}

int SimNode::Read(dataDirection dir, char* bytes, int len)
{
	int sock;
	if (dir == INBOUND)
		sock = insocket;
	else if(dir == OUTBOUND)
		sock = outsocket;
	else
		sock = dir;

	int br;
	char *p = bytes;
	while(len > 0)
	{
		br = read(sock,p,len);
		if(br < 0)
		{
			perror("error read");
			return -1;
		} else if (br == 0) {
			fprintf(stderr,"connection closed\n");
			exit(1);
			return 0;
		}		
		p = p + br;
		len = len - br;
	}
	return bytes - p;
}

int SimNode::Send(dataDirection dir, char* buf, int len)
{
	int sock;
	if (dir == INBOUND)
		sock = insocket;
	else if(dir == OUTBOUND)
		sock = outsocket;
	else sock = dir;
	
	int bs = send(sock, buf, len, 0);
	return bs;
}



