
#include "idltypes.h"
#include "simnode.h"


using namespace std;

#include <deque>
#include <vector>

#define TICK_TIMEOUT 1200000

enum {
	//Ant Message types
	ANT = 0,
	ANT_PONG = 1,
	ANT_REGULAR = 2,
	ANT_UNIFORM = 3,
	//Connection Establishment
	POKE = 4,
	POKE2 = 5,
	RENEWAL = 6,
	PRINT = 7,//debug 
	STATE_EXPLORE = 50,
	STATE_POLICY_Y = 55,
	STATE_HIGHEST = 60
};


class AntNode : public SimNode
{
public:
	AntNode(int port);
	virtual ~AntNode();
public:
	int32 timer_expired(int32 data);
	int32 read_msg(int16 from_node, int16 port_id, bytearray data);
	int32 terminate_node();
protected:
	virtual int32 init_node(int16 node_id, int16 nnodes, int16 nports, 
					int32 simulator_ip, int16 simulator_port);
 	virtual int16 get_rtable_port(int16 dest_node);
	virtual int32 get_rtable_cost(int16 dest_node);


protected:
	int32 get_low_cost(int16 dest_node, int16 port = -1);
	int32 get_avg_cost(int16 dest_node, int16 port = -1);
	int32 get_highest_cost(int16 dest_node, int16 port = -1);

protected:

	struct ant
	{
		unsigned short cost:7;
		unsigned short regular:1;
		unsigned short source:4;
		unsigned short dest:4;
		unsigned short uniq;
	};

	struct AntNode::ant* generate_ant(short dest,bool regular);
	struct AntNode::ant* generate_reply(struct AntNode::ant* pong,short port);

/// Sends an ANT over port,
// regular true = ANT
// regular false = ANT_PONG
	void send_ant(struct AntNode::ant &ANT,short port,bool regular);
	double nudge_factor(short dest, short cost);

// Incoming ANT from port s_port, 
	//int forward_ant(struct ant *ANT,unsigned short s_port,unsigned short d_node, unsigned short &d_dport);
	short forward_port(struct AntNode::ant *ANT, unsigned short s_port);
	void forward_pong(struct AntNode::ant *ANT,unsigned short s_port);
/*  ant_table ( states )
 
 *  Number of ants sent to destination
 *  Number of ants returned ( Pong ) 
 *  Number of dropped ants ( loops )
 */
	struct ninfo
	{
		deque<struct AntNode::ant> ant_cache; //history of ants sent over a given interface destinated for a node
		deque<struct AntNode::ant> cost_history; //only past 5 are kept
               short cost; //records lowest cost observed 
	       	//pheremone trails should degrade over time
	       //Statistics
               short a_sent;
               unsigned short a_uni_sent;
               unsigned short a_reg_sent;
               short a_returned;
               short a_loops;
               float probability;
    	};

	struct port_info 
	{
		//this variable holds meaning about if a port is down, it is -1
		short node; // new ninfo[nnodes]
		short cost;
		unsigned long last_poke_time;
	};

	port_info *port_to_node;


	struct routes //indexed by port
	{
		ninfo *dest; // indexed by node id
		short all_loops;
		short cost;   //cost of the link port
		
	};

	routes *routing_table;
/* Routing tables holds the phereomone trails deposited by the ants
 */
	int *node_to_port; //indexed by nodes, maps to port

	void COST_UPDATE();  //periodically aggregates ant info into probabilities
private:
	void renewal();

protected:
	void print_ant_table();
	void print_port();
	void print_ant_statistics();

	short random_port(struct AntNode::ant *ANT,short dest, short s_node = -1);


private:
	short seq;
	unsigned short uni_sent;
	unsigned short reg_sent;

	bool propagate;
	int tmr_ant_uniform;
	int tmr_ant_regular;
	int tmr_link_err;
	int tmr_pokeports;
	int tmr_print;

	int32 tid;
	int renewal_epoch;
};
