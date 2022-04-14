#include "ns3/Level_flex.h"
#include "ns3/Flow_pl.h"
#include <cmath>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include "ns3/ipv6-header.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/netanim-module.h"
#include "ns3/stats-module.h"

using namespace std;

namespace ns3 {

    class SpPifo : public QueueDisc {

    private:

	static const int DEFAULT_PQ = 25; // # of fifo queues
        static const int DEFAULT_VOLUME = 20; // fifo size
        static const int RANK_RANGE = 1000; // max rank difference value
        static const int DEFAULT_WEIGHT = 2; // weight
        static const int DEFAULT_BURSTNESS = 1000; // burstness
	uint32_t inv_count = 0;
	uint64_t inv_mag = 0;

	typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;
	vector<string> Flowlist; 
	vector<int> weights;
     	std::map<int, int> qpkts; //those pkts who are in the scheduler <uid, rank>
   
	Queue* fifos[DEFAULT_PQ]; // queues
	int bounds[DEFAULT_PQ] = {0}; // initialize all queue bounds to be 0

	int size = 0;
	int drop = 0;
	int uid = 0;
	int maxUid = 0;
	int currentRound = 0;
	int flowNo = 0;

	int enque = 0;
	int deque = 0;
	int empty = 0;
	int depkt = 0;
	int enpkt = 0;

	Flow_pl* getFlowPtr(string fid); 
        Flow_pl* insertNewFlowPtr(string fid, int fno, int weight, int burstness); 
        int updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr); 
        string convertKeyValue(string fid);
	int RankComputation(QueueDiscItem* item, Flow_pl* currFlow);
	string getFlowLabel(Ptr<QueueDiscItem> item);

    public:

        /**
        * \brief Get the type ID.
        * \return the object TypeId
        */
        static TypeId GetTypeId(void);

        /**
        * \brief Gearbox_pl_fid_flex constructor
        * Creates a queue with a default or customed number of packets
        */
        SpPifo();
        virtual ~SpPifo();

        bool DoEnqueue(Ptr<QueueDiscItem> item);
        Ptr<QueueDiscItem> DoDequeue(void);
 	Ptr<QueueDiscItem> FifoDequeue(int);
	Ptr<QueueDiscItem> PifoDequeue();
        Ptr<const QueueDiscItem> DoPeek(void) const;
        Ptr<QueueDiscItem> DoRemove(void);

	int GetQueueSize(int);
	void FifoPrint(int);
	void Record(string fname, Ptr<QueueDiscItem> item);

        bool CheckConfig(void);
        void InitializeParams(void);
        uint32_t m_limit;    //!< Maximum number of packets that can be stored

	int addSchePkt(int uid, int departureRound); // return pkts count
        int removeSchePkt(int uid); // return pkts count
        int cal_inversion_mag(int departureRound); //return inversion magnitude, if 0 then there is no inversion

    };
}

