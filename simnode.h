
#include "idltypes.h"
#include "nodeinterface.h"

using namespace std;

#include <deque>

#define TICK_TIMEOUT 1200000


template <typename T>
inline T MIN(T a, T b) { return(a < b ? a : b); }

// Node class
class SimNode : public NodeInterface
{
public:
	SimNode(int port);	
	virtual ~SimNode();

protected: 
	int Read(dataDirection dir, char* bytes, int len);
	int Send(dataDirection dir, char* data, int len);
	
	// Outbound functions redefining interface virtuals!
	virtual int32 init_node(int16 node_id, int16 nnodes, int16 nports, 
					int32 simulator_ip, int16 simulator_port);
    	virtual int32 timer_expired(int32 data);
	virtual int32 read_msg(int16 from_node, int16 port_id, bytearray data);
	virtual int32 terminate_node();
	virtual int16 get_rtable_port(int16 dest_node);
	virtual int32 get_rtable_cost(int16 dest_node);

protected:
	int insocket;		// inbound connection
	int outsocket;		// outbound connection
	int listener;		// outbound connection

	int16 node_id;
	int16 nnodes;
	int16 nports;

	int32 simulator_ip;
	int32 simulator_port;

};
