#include "ns3/Level_flex_pFabric.h"

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

    class pFabric : public QueueDisc {

    private:

        static const int DEFAULT_VOLUME = 1; // number of levels
        static const int FIFO_PER_LEVEL = 25;  // number of fifos per level
        static const int DEFAULT_WEIGHT = 2;   // default weight
        static const int DEFAULT_BURSTNESS = 20000;  // default burstness
 	static const int SPEEDUP_FACTOR = 4; // default speed up
        static const int GRANULARITY = 800; // granularity per fifo
	static const int DEFAULT_FIFO_N_SIZE = 20;  //per flow queue

        int volume;  // num of Level_flexs in scheduler
        int currentRound;  // current Round
        int pktCount;  // packet count in Gearbox
	int maxUid; // max pkt uid among all pkts coming into Gearbox
        int enqueCount; // # of enqueue operations
        int dequeCount; // # of dequeue operations
        int reloadCount = 0; // # of ptks reload operations
        int dropCount = 0; // total # of dropped pkts
        int dropCountA = 0; // # of dropped pkts because of too large range
        int dropCountB = 0; // # of dropped pkts because of fifo overflow
        int dropCountC = 0; // # of dropped pkts which had been enqueued into pifo or fifo
	uint32_t inv_count = 0;
	uint64_t inv_mag = 0;

	int uid = 0; // the last uid
	vector<int> flowsize;
	//std::map<int, vector<int>> packetList;
	Queue* Per_flow_queues[10000];
	typedef std::map<int, bool> EnqueMap;//record whether each flow is enqueued to scheduler
        EnqueMap enqueMap;
	typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;
	vector<string> Flowlist; 
        Level_flex_pFabric levels[DEFAULT_VOLUME]; // levels
	uint64_t rankDifference[1500] = {0}; // the difference between the current round and the incoming pkt's rank
	//vector<int> weights; // FIFO_PER_LEVEL * DEFAULT_FIFO_SIZE + PIFO_SIZE at most 

        void setPktCount(int);
        Flow_pl* getFlowPtr(string fid); 
        Flow_pl* insertNewFlowPtr(string fid, int fno, int weight, int flowsize, vector<int> packet_List,int burstness);
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
        * \brief pFabric constructor
        * Creates a queue with a default or customed number of packets
        */
        pFabric();
        explicit pFabric(int);
        virtual ~pFabric();

        int flowNo;
	ifstream flowf;
	vector<int> weights;

        bool DoEnqueue(Ptr<QueueDiscItem> item);
        Ptr<QueueDiscItem> DoDequeue(void);
 	//Ptr<QueueDiscItem> FifoDequeue(int);
	Ptr<QueueDiscItem> PifoDequeue();

        Ptr<const QueueDiscItem> DoPeek(void) const;
        bool CheckConfig(void);
        void InitializeParams(void);
        uint32_t m_limit;    //!< Maximum number of packets that can be stored

        Ptr<QueueDiscItem> DoRemove(void);
        void setCurrentRound(int);
        int cal_theory_departure_round(Ipv4Header, int);
        int cal_insert_level(int, int);
	int cal_index(int, int); // calculate the index according to the pkt's departureRound and the level
        void setEnqueCount(int count);
        void setDequeCount(int count);
        void setDropCount(int count);
        int getEnqueCount(void);
        int getDequeCount(void);
        int getDropCount(void);

	void ifLowerThanLthenReload(int level);
	void Record(string fname, int departureRound, Ptr<QueueDiscItem> item);
	void tagRecord(int uid, int departureRound, Ptr<QueueDiscItem> item);
	void Reload();
	void RecordFlow(string flowlabel, int departureRound, int uid, string filename);
	void RecordFCT(string filename, int index, double FCT);
	void AddTag(int flowNo, int departureRound, int uid, QueueDiscItem* item);
    };



}

