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

    class Gearbox_pl_fid_flex : public QueueDisc {

    private:

        static const int DEFAULT_VOLUME = 1; // number of levels

        static const int FIFO_PER_LEVEL = 10;  // flex level

        static const int DEFAULT_WEIGHT = 2;   // default weight

        static const int DEFAULT_BURSTNESS = 1000;  // default burstness

 	static const int SPEEDUP_FACTOR = 5; // default speed up


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

	int uid = 0; // the last uid

	typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;
	vector<string> Flowlist; 
        Level_flex levels[DEFAULT_VOLUME]; // levels
	uint64_t rankDifference[1500] = {0}; // the difference between the current round and the incoming pkt's rank

        void setPktCount(int);
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
        Gearbox_pl_fid_flex();
        explicit Gearbox_pl_fid_flex(int);
        virtual ~Gearbox_pl_fid_flex();

        int flowNo;
	ifstream flowf;
	vector<int> weights;

        bool DoEnqueue(Ptr<QueueDiscItem> item);
        Ptr<QueueDiscItem> DoDequeue(void);
 	Ptr<QueueDiscItem> FifoDequeue(int);
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
	void RecordFlow(uint64_t flowlabel, int departureRound);
	void AddTag(bool level, int departureRound, int uid, QueueDiscItem* item);
    };



}

