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

#include "antnode.h"


AntNode::AntNode(int port) : SimNode(port)
{
	struct timeval tv;

	gettimeofday(&tv,NULL);
	srandom(tv.tv_sec + tv.tv_usec);
	propagate = false;
}

AntNode::~AntNode()
{
	int i;
	for(i = 0; i < nports;i++)
	{
		delete [] routing_table[i].dest;
	}
	delete [] routing_table;
	delete [] port_to_node;
}

// Outbound functions
int32 AntNode::init_node(int16 _node_id, int16 _nnodes, int16 _nports, 
					int32 _simulator_ip, int16 _simulator_port)
{
	int i,j, now;
	SimNode::init_node(_node_id, _nnodes, _nports, _simulator_ip, _simulator_port);

	routing_table = new routes[nports];
	for(i = 0;i < nports;i++)
	{	
			routing_table[i].dest = new ninfo[nnodes];
	}

	printf("DATA NODE: %d\n",node_id);
	//
	//
	//
	//the rest of default values are set in the renewal function, that gets called by a timer.
	uni_sent = 0;
	reg_sent = 0;
	for(i = 0;i < nports;i++)
	{	
			for(j = 0;j < nnodes;j++)
			{
				routing_table[i].dest[j].cost = -1;
				routing_table[i].dest[j].a_sent = 0;
				routing_table[i].dest[j].a_uni_sent = 0;
				routing_table[i].dest[j].a_reg_sent = 0;
				routing_table[i].dest[j].a_returned = 0;
				routing_table[i].dest[j].a_loops = 0;
				routing_table[i].dest[j].probability = 1 / float(nports);
				if(node_id == j)
					routing_table[i].dest[j].probability = 0;
				routing_table[i].dest[j].ant_cache.clear();
			}
			routing_table[i].all_loops = 0;
	}

	port_to_node = new port_info[nports];
	for(i = 0;i < nports;i++)
	{
		port_to_node[i].node = -1;
		port_to_node[i].cost = -1;
	}
	// Why don't I just include this in my routing_table


	now = stub_get_current_time();

	tmr_ant_regular = stub_add_timer(now + 10000 + node_id, ANT_REGULAR); //stagger the initial timers
	tmr_ant_uniform = stub_add_timer(now + 10000 + node_id, ANT_UNIFORM); //stagger the initial timers
//	stub_add_timer(now + 300000000 + node_id, RENEWAL);
	//tmr_link_err = stub_add_timer(now + 800000000 + node_id, STATE_EXPLORE);

	tmr_pokeports = stub_add_timer(now + 4000, POKE); //makes sure ports are active
	tmr_print = stub_add_timer(now + 15000,PRINT);
	//Should take into account last link failure

	seq = 0;
	renewal_epoch = 0;

	reg_sent = 0;
	uni_sent = 0;

	return 0;
}


int32 AntNode::timer_expired(int32 data)
{
	int i;
	int now;
	short dest;
	short port;
	struct ant* n;
	int try_ = -20;

	now = stub_get_current_time();
	
	if(data == ANT_UNIFORM)
	{
		dest = /*random()*/ uni_sent++ % nnodes;
		//AntNET suggests sending ants proportinally to data flow
		//
		do {
			port = random() % nports;

		} while(port_to_node[port].node == -1 && try_++); // || (.90 * routing_table[port].dest[dest].a_sent) < routing_table[port].dest[dest].a_loops ); 

		if(dest != node_id && port_to_node[port].node != -1)
		{
			n = generate_ant(dest,false);
			printf("NEW UNI ANT UNIQ[%d] DEST[%d] PORT [%d]\n",n->uniq, dest,port);

			n->cost = port_to_node[port].cost;
			send_ant(*n,port,true);
			routing_table[port].dest[dest].a_sent++;
			routing_table[port].dest[dest].a_uni_sent++;
			routing_table[port].dest[dest].ant_cache.push_back(*n);
			delete n;
		}

		tmr_ant_uniform = stub_add_timer(now + 30000000, ANT_UNIFORM);
		
	} else if(data == ANT_REGULAR)
	{
		int i;

		dest = reg_sent++ % nnodes;
		if(dest != node_id)
		{
			n = generate_ant(dest,true);
			port = random_port(n,dest);
		
			if(port != -1)
			{
				n->cost = port_to_node[port].cost;
				printf("NEW REG ANT UNIQ[%d] DEST[%d] PORT [%d]\n",n->uniq, dest,port);
				send_ant(*n,port,true);
				routing_table[port].dest[dest].a_sent++;
				routing_table[port].dest[dest].a_reg_sent++;
				routing_table[port].dest[dest].ant_cache.push_back(*n);
			}
			delete n;
		}
		

		tmr_ant_regular = stub_add_timer(now + 1800000, ANT_REGULAR);
	
		
	} else if(data == POKE)
	{
		int i;
		bytearray msg;
		short *type;


		msg.len = sizeof(short);
		msg.data = new char[msg.len];


		type = (short *)msg.data;
		*type = POKE;

		//printf("ant table before POKE\n");
		//print_ant_table();
		
		for(i = 0;i < nports;i++)
		{
			if(now - port_to_node[i].last_poke_time > TICK_TIMEOUT)
			{
				port_to_node[i].node = -1;
				port_to_node[i].cost = -1;
			}

			stub_write_msg(i,msg);	
		}

		stub_add_timer(now + 300000, POKE);
		//stub_add_timer(now + 5000, POKE2);
		delete msg.data;

	} else if(data == POKE2) {
		int i,j,k,d;
		int lowest_cost;
		int lowest_port;

		for(j = 0;j < nnodes;j++)
		{
			lowest_cost = 999;
			lowest_port = -1;
			
			for(i = 0;i < nports;i++)
			{
				if(port_to_node[i].node == j)
				{
					if(port_to_node[i].cost < lowest_cost)
					{
						lowest_cost = port_to_node[i].cost;
						lowest_port = i;
					}
				}
			}
			if(lowest_port != -1)
			for(i = 0;i < nports;i++)
			{
				if(port_to_node[i].node == j && lowest_port != i)
				{
					port_to_node[i].node = -1;
					port_to_node[i].cost = -1;
					for(d=0;d< nnodes;d++)
						routing_table[i].dest[d].probability = 0;
						// DONT FORGET TO NORMALIZE OTHER PROBABILITIES
				}
			}
		}
		printf("ant table after POKE2\n");
		print_ant_table();

		stub_add_timer(now + 50000000, POKE);
	} else if(data == PRINT){
		print_ant_table();
		stub_add_timer(now + 5000000, PRINT);
	} else if(data == RENEWAL){
		printf("Before renewal\n");
		print_ant_table();
		// updates the costs according to probabilities
		COST_UPDATE();
		printf("After renewal\n");
		print_ant_table();

		renewal_epoch = 35;
		stub_add_timer(stub_get_current_time() + 30*3000000 + node_id, RENEWAL);
	
	}

	return 0;
}




int32 AntNode::read_msg(int16 from_node, int16 port_id, bytearray data)
{
	int i; 
	short *type;
	struct ant* inbound;
	struct ant* outbound;

	type = (short *) data.data;

	inbound = (struct ant *) (data.data + sizeof(short));

	//fprintf(stderr,"Node %d read_msg from %d over port %d\n",node_id,from_node,port_id);

	
	if(*type == ANT)
	{
		if(inbound->dest == node_id)
		{
			printf("ANT HIT UNIQ[%d] %d -> %d [cost %d]\n",inbound->uniq,inbound->source,node_id,inbound->cost);
			outbound = generate_reply(inbound,port_id);
			send_ant(*outbound,port_id,false);
		} else {
			int lastinterface = port_id;
			short n_port;


			//to what destination should I pick?
			n_port = forward_port(inbound,port_id);  //returns -1 if the ant looped.
			if(n_port != -1)
			{
				printf("node[%d] UNIQ[%d] FORWARDING inport=%d source=%d destination=%d outport=%d COST %d\n",node_id,inbound->uniq,
					port_id,inbound->source, inbound->dest, n_port, inbound->cost);
				inbound->cost += port_to_node[n_port].cost;
				if(port_to_node[n_port].cost == -1)
					printf("SHIT, DONT USE THAT PORT\n");
				printf("node[%d] UNIQ[%d] FORWARDING inport=%d source=%d destination=%d outport=%d COST %d\n",node_id,inbound->uniq,
					port_id,inbound->source, inbound->dest, n_port, inbound->cost);
				send_ant(*inbound,n_port, true);
			}


			//now need to find the port that we are going to forward this ant over.  it came from node_id, over port_id
		}
		//Am I the destination?, send ANT_PONG
	} else if(*type == ANT_PONG) {
		short n_port;
		int total_cost = 0,avgcost = 0,avgother = 0;
		int k;  // number of active ports
/*  if the ant is a pong, we first check to see the pongs destination.  if it is ourselves, then an ant has completed
 *  it's first run to the destination.  Incriment the number of ants returned over that interface by one.  and adjust the prob
 * of reaching that destination over all other destinations by P' = (P + d) / (d + 1)
 *

 * the running cost to destination is avgcost of returning ant, over all other ports
*/

		if(inbound->dest == node_id)
		{
			double p_delta = 0.285; //this is gamma
		
			//p_delta = nudge_factor(inbound->dest, inbound->cost);

			printf("node[%d] UNIQ[%d] ANT RETURN PORT %d DEST %d COST %d\n",node_id,inbound->uniq,
				port_id, inbound->source, inbound->cost);
				

			routing_table[port_id].dest[inbound->source].a_returned++;
			//normallize probabilites over ports... take cost into account

			while(routing_table[port_id].dest[inbound->source].cost_history.size() > 20)
				routing_table[port_id].dest[inbound->source].cost_history.pop_front();
	// favor lower costing ports.
			for(i = 0;i < nports;i++)
			{
				float *prob = &(routing_table[i].dest[inbound->source].probability);
				//if(routing_table[i].dest[inbound->source].cost == -1) //inactive ports
				//	continue;

				if(i == port_id) //increase probability reaching destination over this port
				{
					*prob = (*prob + p_delta) / (p_delta + 1.00);
				} else {// lower probability for other ports
					*prob = (*prob) / (p_delta + 1.00);
				}


				avgcost = get_avg_cost(inbound->source);
			}
			routing_table[port_id].dest[inbound->source].cost_history.push_back(*inbound);



// keep a running total of atmost (20) ants sent/returned



			avgcost = get_avg_cost(inbound->source,port_id); //inbound->cost;
			//avgcost is the mean cost to reach source over port_id
			avgother = get_avg_cost(inbound->source);
			//routing_table[port_id].dest[inbound->source].cost = avgcost;

			if(avgother != -1 && avgother != 0)
			{
				routing_table[port_id].dest[inbound->source].cost = ceil((1.000 - (double)avgcost)/(double)avgother);
				
			}
			

			k++;	
		} else {
			forward_pong(inbound,port_id);
		}


	} else if (*type == POKE) { 
		port_to_node[port_id].node = from_node;
		port_to_node[port_id].cost = stub_get_cost(port_id);
		port_to_node[port_id].last_poke_time = stub_get_current_time();
		
	}

	//fprintf(stderr,"Node %d -----\n",node_id);

	return 0;
}

int32 AntNode::terminate_node()
{
	SimNode::terminate_node();
	printf("AntNode: Terminate node\n");
	print_ant_table();
	print_ant_statistics();
	return 0;
}


int16 AntNode::get_rtable_port(int16 dest_node)
{
	int ret = -1;
	int i;
	int highest = 999;

	int avgcost = 999;
	int avgother = 999;

	for(i = 0;i < nports;i++)
	{
		if(routing_table[i].dest[dest_node].cost == -1 || port_to_node[i].node == -1)
			continue;
		if((avgcost = get_avg_cost(dest_node,i)) < highest)
		{
			highest = avgcost;
			ret = i;
		}
	}


	return ret;
}

int32 AntNode::get_rtable_cost(int16 dest_node)
{
	int i;
	int lowest = 999, lp = -1;


	for(i=0;i< nports;i++)
	{
		int cost;// = routing_table[i].dest[dest_node].cost;
		if(routing_table[i].dest[dest_node].cost == -1 || port_to_node[i].node == -1)
			continue;
			cost = get_avg_cost(dest_node,i);
		if(cost == -1 || cost == 101)
			continue;
		if(cost < lowest)
		{
			lowest = cost;
			lp = i;
		}

	}

	return lowest;

}

void AntNode::send_ant(struct ant &dest,short port, bool ping)
{
	short *type;
	char *ptr;
	bytearray data;
	data.len = sizeof(short) + sizeof(struct ant);
	data.data = new char[data.len];
	bzero(data.data,data.len);

	type = (short *) data.data;
	ptr = data.data + sizeof(short);
	memcpy(ptr,(struct ant *) &dest,sizeof(struct ant));

	if(ping)
		*type = ANT;
	else
		*type = ANT_PONG;

	stub_write_msg(port,data);

	delete [] data.data;
	return;
	
}


struct AntNode::ant *AntNode::generate_reply(struct AntNode::ant* ping, short interface) 
{
	struct AntNode::ant* pong = new struct ant;

	if( !ping )
	{
		fprintf(stderr,"bad ping\n");
		delete pong;
		return NULL;
	}
	pong->cost = ping->cost;
	pong->regular = ping->regular;
	pong->source = node_id;
	pong->dest = ping->source;
	pong->uniq = ping->uniq;

	return pong;
}



/* Sets probability proportional to the number of average ant cost
 *
 */
void AntNode::COST_UPDATE()
{
	int i,j,k = 0;
	int total_cost;

	for(j= 0;j < nnodes;j++)
	{
		total_cost = 0;
		for(i=0;i< nports;i++)
		{
			if(port_to_node[i].node == -1|| node_id == j)
			{
				routing_table[i].dest[j].probability  = 0.00;
				continue;
			}
			

			routing_table[i].dest[j].cost = get_avg_cost(j,i);
			if(routing_table[i].dest[j].cost != -1)
			{
				total_cost += routing_table[i].dest[j].cost;
				k++;
			} else {
				routing_table[i].dest[j].probability = 0.00;
			}
			
		}
			

		for(i=0;i< nports;i++)
		{
			int cost = routing_table[i].dest[j].cost;
			printf("Average cost: %d total_cost: %d\n",routing_table[i].dest[j].cost,total_cost);
			if(routing_table[i].dest[j].cost == -1)
				continue;
			printf("node[%d] cost: %d\n", j, cost);
			routing_table[i].dest[j].probability = (1.000 - float(cost)/total_cost + 1.000);
		}
	

	}
}

int32 AntNode::get_highest_cost(int16 dest_node, int16 port)
{
	int ret = 1;
	int i, k = 0;
	int *cost;
	int sum = 0, cntr = 0;
	int highest = 999;
	int lowest = 999;




	for(i = 0;i < nports;i++)
	{
		if(routing_table[i].dest[dest_node].cost == -1)
			continue;
		if(routing_table[i].dest[dest_node].cost < highest)
		{
			highest = routing_table[i].dest[dest_node].cost;
			ret = i;
		}
	}
	return highest;


}

int32 AntNode::get_low_cost(int16 dest_node, int16 port)
{
	int ret = 1;
	int i, k = 0;
	int *cost;
	int sum = 0, cntr = 0;
	int highest = 999;
	int lowest = 999;



	
	cost = new int[nports];
	for(i = 0;i < nports;i++)
		cost[i] = -1;
	
	deque<struct ant>::iterator it;


	for(i = 0;i < nports;i++)
	{
		sum = 0;
		cntr = 0;
		if(port_to_node[i].node == -1)
			continue;
		for(it = routing_table[i].dest[dest_node].cost_history.begin(); it != routing_table[i].dest[dest_node].cost_history.end();it++)
		{
			if (lowest > (*it).cost )
				lowest = (*it).cost;
			sum += (*it).cost;
			cntr++;
		}
		if(port != -1 && port == i)
		{
			if(cntr == 0)
			{
				return 101;
			}
			 return lowest;
		}


		if (cntr != 0)
		{
			cost[i] = lowest; //sum/cntr;
			k++;
		}
		else 
		{
	//		routing_table[i].dest[dest_node].cost = -1;
			//port has no cost history, port is dead, or need to send more ants
		}
	}


	sum = 999;

	for(i = 0;i < nports;i++)
	{
		if( cost[i] == -1)
			continue;
		if(cost[i] < sum)
		{
			sum = cost[i];
			ret = i;
		}
	}
	

	
	return cost[ret];
}


int32 AntNode::get_avg_cost(int16 dest_node, int16 port)
{
	int ret = 1;
	int i, k = 0;
	int *cost;
	int sum = 0, cntr = 0;
	int highest = 999;
	int lowest = 999;



	
	cost = new int[nports];
	for(i = 0;i < nports;i++)
		cost[i] = -1;
	
	deque<struct ant>::iterator it;


	for(i = 0;i < nports;i++)
	{
		sum = 0;
		cntr = 0;
		if(port_to_node[i].node == -1)
			continue;
		for(it = routing_table[i].dest[dest_node].cost_history.begin(); it != routing_table[i].dest[dest_node].cost_history.end();it++)
		{
			if((*it).regular == false)
				continue; //don't count uniform ants
	
			if (lowest > (*it).cost )
				lowest = (*it).cost;
			sum += (*it).cost;
			cntr++;
		}
		if(port != -1 && port == i)
		{
			if(cntr == 0)
			{
				return 101;
			}
			 return sum/cntr;
		}


		if (cntr != 0)
		{
			cost[i] = sum/cntr;
			k++;
		}
		else 
		{
	//		routing_table[i].dest[dest_node].cost = -1;
			//port has no cost history, port is dead, or need to send more ants
		}
	}


	sum = 999;

	for(i = 0;i < nports;i++)
	{
		if( cost[i] == -1)
			continue;
		if(cost[i] < sum)
		{
			sum = cost[i];
			ret = i;
		}
	}
	

	
	return cost[ret];
}



// if the ant is uniform we select a port at random from the available ports
// if the ant is regular, we select the port based on the probability that the ant will get there..
short AntNode::forward_port(struct AntNode::ant *critter, unsigned short s_port)
{
	int i,j,k = 0;
	float highest = 0;
	int total_cost = 0;
	short f_port = s_port;
	unsigned int d_node = critter->dest;

	//first check to see if the ping was sent before...
	for(i = 0;i < nports;i++)
	{
		
		deque<struct ant>::iterator it;


		if(port_to_node[i].node == -1)
			continue;
		if(port_to_node[i].cost == -1)
			continue;

        	for(it = routing_table[i].dest[d_node].ant_cache.begin(); it != routing_table[i].dest[d_node].ant_cache.end(); it++)
		{
			if(it->uniq == critter->uniq && it->dest == critter->dest && it->source == critter->source)
			{ // loop detected
				
				routing_table[i].all_loops++;
				if(node_id == critter->source)
					routing_table[i].dest[d_node].a_loops++;
				routing_table[i].dest[d_node].ant_cache.erase(it);
				printf("DROPPED ANT UNIQ[%d]\n",critter->uniq);
				return -1;
			}	
		}
	}
	
	// assign a random port for the ant to be forwarded on based regular ant, uniform ant, and port statuses
	f_port = random_port(critter,d_node,s_port);
	
	if(f_port != -1)
	{
		routing_table[s_port].dest[d_node].ant_cache.push_back(*critter); //keep track of the ant that was forwarded so that PONG will be sent along the same path.

	}
	// return s_port if there are no other ports available.
	return f_port;
}

void AntNode::forward_pong(struct AntNode::ant *ANT,unsigned short s_port)
{
	int i;
	unsigned short s_node = ANT->source;

/// loop through the routing table and forward the ant out the interface found 

	for(i = 0;i < nports;i++)
	{
		deque<struct ant>::iterator it;
        	for(it = routing_table[i].dest[s_node].ant_cache.begin(); it != routing_table[i].dest[s_node].ant_cache.end(); it++)
		{
			if((*(it)).uniq == ANT->uniq)  
			{
				//found ant, forward pong 
				send_ant(*ANT,i,false);
				routing_table[i].dest[s_node].ant_cache.erase(it);
				

				return;
			}
		}
		
	}
	printf("Could not find ping for which pong to send\n");
	
	//search through the cache to find the ping so that we know what node to send it over;
	//routing_table[s_port].dest[s_node].ant_cache
}


void AntNode::print_ant_table()
{
	int i,j;


	for(j = 0;j < nnodes;j++)
	{
		printf("DATA NODE%d ", j);
		for(i = 0;i < nports;i++)
		{
			float print_probab = routing_table[i].dest[j].probability*100.00;
			printf("%0.4f ",print_probab);
		}
		printf("\n");
	}
	printf("\n");
}

void AntNode::print_ant_statistics()
{
	int i,j;
	for(i=0;i < nnodes;i++)
	{
		for(j = 0;j < nports;j++)
		{
			printf("ANTSTAT DEST%d PORT%d ANTSSENT%d\n",i,j, routing_table[j].dest[i].a_sent);
			printf("ANTSTAT DEST%d PORT%d UNISENT%d\n",i,j, routing_table[j].dest[i].a_uni_sent);
			printf("ANTSTAT DEST%d PORT%d REGSENT%d\n",i,j, routing_table[j].dest[i].a_reg_sent);
			printf("ANTSTAT DEST%d PORT%d RETURNED%d\n",i,j, routing_table[j].dest[i].a_returned);
			printf("ANTSTAT DEST%d PORT%d LOOPED%d\n",i,j, routing_table[j].dest[i].a_loops);
		}
	}
}

struct AntNode::ant * AntNode::generate_ant(short dest,bool regular)
{
	struct ant *n_ant;
	unsigned short ant_id;

	n_ant = new struct ant;
	n_ant->cost = 0; //default it to the cost of going out that port
					//but we don't know that yet
	n_ant->regular = regular; // true for regular ant, false for uniform ant
	n_ant->source = node_id; // 4 bits
	n_ant->dest = (unsigned) dest; // 4 bits

	ant_id = node_id;
	ant_id = ant_id << 12;

	ant_id += seq;

	n_ant->uniq = ant_id; 
	seq++;

	if(seq >= 2048)
	{
		seq = 0;
	}

	return n_ant;
}



// based on the waited probabilities, it picks a destination port to forward ant
//returns s_node if no available ports could be picked
short AntNode::random_port(struct ant *ANT, short dest, short s_node)
{

	short f_port = s_node;	
	int i,k = 0;

	float highest = 0;

	int total_probability = 0;
	float *probab = new float[nports];
	short *probport = new short[nports];

	for(i = 0;i < nports;i++)
	{
		probab[i] =0;
		probport[i] = -1;
	}
	

	for(i = 0;i < nports;i++)
	{
		if(s_node == i) //don't pick the inbound port
			continue;
		if(port_to_node[i].node == -1) //invalid sockets have no nodes attached
			continue;
		if(routing_table[i].dest[dest].probability <= 0.001)
			continue;

		//if(routing_table[i].dest[dest].a_loops == routing_table[i].all_loops && routing_table[i].all_loops > 10) 
		//	continue; //if we've had all loops, ignore port for that destination

		probab[k] = routing_table[i].dest[dest].probability;
		total_probability += int(probab[k]*100.000);
		probport[k] = i;
		k++;
	}

	if(ANT->regular && k != 0)
	{
		int total_prob = total_probability;
		for(i = 0;i < k;i++)
		{
			total_prob -= int((probab[i]*100.000));
			if((random() % 100) > total_prob )
			{
				f_port = probport[i];
				break;
			}

			if(i == k)
			{
				printf("invalid chance");
			}
		}
		
	} else if(k != 0) {
		i = random() % k;
		f_port = probport[i];
		
	}


	delete probab;
	delete probport;

	return f_port;
}


//this is the adjust probability that is done over all the links
double AntNode::nudge_factor(short dest, short cost)
{
	int avg_cost;
        return 0.85;

	/*if((diff = (cost - get_avg_cost(dest))) <= 0)
	{
		return -0.035*diff;
	} else if(diff == 0) {
		return 0.3;
	} else if(diff == cost + 1) {
		return 0.011;
	} else {
		return 0.001*diff;


	}*/
	if( (avg_cost = get_avg_cost(dest)) == -1 )
	{
		return 0.011;
	} else if( avg_cost > cost ) {
		return 0.120;
	} else if (avg_cost == cost) {
		return 0.085;
	} else {
		return 0.015;
	}
}

