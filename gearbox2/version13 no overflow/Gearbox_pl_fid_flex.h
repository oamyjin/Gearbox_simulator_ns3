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

        static const int DEFAULT_VOLUME = 3;

        static const int FIFO_PER_LEVEL = 10;         // 01212020 Peixuan flex level

        static const int DEFAULT_WEIGHT = 2;         // 01032019 Peixuan default weight

        static const int DEFAULT_BRUSTNESS = 1000;    // 01032019 Peixuan default brustness

        static const int TIMEUNIT = 1;    // 01032019 Peixuan default brustness

 	static const int SPEEDUP_FACTOR = 2;

        int volume;                     // num of Level_flexs in scheduler

        int currentRound;           // current Round

        int pktCount;           // packet count

	

	int cum_pktCount;

        int enqueCount;

        int dequeCount;

	int migrationCount=0;

        int dropCount = 0;

	

	int overflowCount = 0;

        int reloadCount = 0;

	 

	double queuingCount = 0;

	Time prevTime1 = Seconds (0);

	

	Time curTime;

	int uid = 0;

	

	int enque_previous, deque_previous, overflow_previous, reload_previous, migration_previous = 0;

	

	int enquerate, dequerate, overflowrate, reloadrate,migrationrate;

	vector<uint64_t> Flowlist; 

        Level_flex levels[3];

        //vector<Flow_pl> flows;

        vector<Ptr<QueueDiscItem>> pktCurRound;

        void setPktCount(int);

        typedef std::map<string, Flow_pl*> FlowMap;

        FlowMap flowMap;

        //typedef std::map<int, Flow_pl*> TestIntMap;

        //TestIntMap testIntMap;

        //12132019 Peixuan

        //Flow_pl* getFlowPtr(nsaddr_t saddr, nsaddr_t daddr);

        Flow_pl* getFlowPtr(uint64_t fid);   // Peixuan 04212020

	uint64_t getFlowLabel(Ptr<QueueDiscItem> item);

        //int getFlowPtr(ns_addr_t saddr, ns_addr_t daddr);

        //Flow_pl* insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness);

        Flow_pl* insertNewFlowPtr(uint64_t fid,int flowNo, int weight, int brustness);   // Peixuan 04212020

        //int updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr);

        int updateFlowPtr(int departureRound, uint64_t fid, Flow_pl* flowPtr);    // Peixuan 04212020

        //string convertKeyValue(nsaddr_t saddr, nsaddr_t daddr);

        string convertKeyValue(uint64_t fid);    // Peixuan 04212020

	int RankComputation(QueueDiscItem* item, Flow_pl* currFlow);

	

    public:

        int m_one;

        int flowNo;

        /**

        * \brief Get the type ID.

        * \return the object TypeId

        */

        static TypeId GetTypeId(void);

        /**

        * \brief Gearbox_pl_fid_flex constructor

        *

        * Creates a queue with a default or customed number of packets

        */

        Gearbox_pl_fid_flex();

        explicit Gearbox_pl_fid_flex(int);

        virtual ~Gearbox_pl_fid_flex();

        bool DoEnqueue(Ptr<QueueDiscItem> item);

	bool PifoEnqueue(int level, QueueDiscItem* item);

	bool FifoEnqueue(QueueDiscItem* item, int index);

        Ptr<QueueDiscItem> DoDequeue(void);

 	Ptr<QueueDiscItem> FifoDequeue(int);

	Ptr<QueueDiscItem> PifoDequeue(int);

	void Migration(int);

        Ptr<const QueueDiscItem> DoPeek(void) const;

        bool CheckConfig(void);

        void InitializeParams(void);

        uint32_t m_limit;    //!< Maximum number of packets that can be stored

        Ptr<QueueDiscItem> DoRemove(void);

        void setCurrentRound(int);

        int cal_theory_departure_round(Ipv4Header, int);

        int cal_insert_level(int, int);

	/*

	   calculate the index according to the pkt's departureRound and the level

	*/

	int cal_index(int, int);

        // Packet* serveCycle();

        // vector<Packet> serveUpperLevel(int &, int);

        void setEnqueCount(int count);

        void setDequeCount(int count);

        void setDropCount(int count);

        int getEnqueCount(void);

        int getDequeCount(void);

        int getDropCount(void);

        int findearliestpacket(int volume);

	void ifLowerThanLthenReload(int level);

	void Record(string fname, int departureRound, Ptr<QueueDiscItem> item);

	void tagRecord(int flowiid, int uid, int departureRound, Ptr<QueueDiscItem> item);

	void PacketRecord(string fname, int departureRound, Ptr<QueueDiscItem> item);

	void Rate(int enqueCount, int dequeCount, int overflowCount, int reloadCount, int migrationCount);

	void Reload();

	void RecordFlow(uint64_t flowlabel, int departureRound);

	bool LevelEnqueue(QueueDiscItem* item, int flowid, int departureRound,int uid);

	void AddTag(int flowid, int departureRound, int uid, QueueDiscItem* item);

	void ifCurrentFifoChange(int i, int currentFifo);

    };



}

