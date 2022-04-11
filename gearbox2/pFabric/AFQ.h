#include "ns3/Level_afq.h"

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

    class AFQ : public QueueDisc {

    private:

        static const int DEFAULT_VOLUME = 1; //number of level

	 static const int GRANULARITY_PER_FIFO = 20000; //1//100000

        static const int FIFO_PER_LEVEL = 1; //1000 //1 //20000     

        static const int DEFAULT_WEIGHT = 2;         

        static const int DEFAULT_BURSTNESS = 20000; //1000   //20000

        static const int TIMEUNIT = 1;   

 	static const int SPEEDUP_FACTOR = 4;

	static const int DEFAULT_FIFO_N_SIZE = 20;  //per flow queue

        int volume;                     // num of Level_flexs in scheduler

        int currentRound;           // current Round

        int pktCount;           // packet count


	int cum_pktCount;

        int enqueCount=0;

        int dequeCount=0;



        int dropCount = 0;
	int dropCountA = 0;
	int dropCountB = 0;

	int enque = 0;
	int deque = 0;
	int drop = 0;
	int empty = 0;
	

	int overflowCount = 0;


	 

	double queuingCount = 0;

	Time prevTime1 = Seconds (0);

	

	Time curTime;

	int uid = 0;

	

	int enque_previous, deque_previous, overflow_previous = 0;

	

	int enquerate, dequerate, overflowrate;

	vector<string> Flowlist; 

        Level_afq levels[DEFAULT_VOLUME];
	Queue* Per_flow_queues[10000];
	typedef std::map<int, bool> EnqueMap;//record whether each flow is enqueued to scheduler
        EnqueMap enqueMap;

        //vector<Flow_pl> flows;

        vector<Ptr<QueueDiscItem>> pktCurRound;

        void setPktCount(int);

        typedef std::map<string, Flow_pl*> FlowMap;

        FlowMap flowMap;

        //typedef std::map<int, Flow_pl*> TestIntMap;

        //TestIntMap testIntMap;

        //12132019 Peixuan

        //Flow_pl* getFlowPtr(nsaddr_t saddr, nsaddr_t daddr);

        Flow_pl* getFlowPtr(string fid);   // Peixuan 04212020

	string getFlowLabel(Ptr<QueueDiscItem> item);

        //int getFlowPtr(ns_addr_t saddr, ns_addr_t daddr);

        //Flow_pl* insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness);

        Flow_pl* insertNewFlowPtr(string fid,int flowNo, int weight, int flowsize, int burstness);   // Peixuan 04212020

        //int updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr);

        int updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr);    // Peixuan 04212020

        //string convertKeyValue(nsaddr_t saddr, nsaddr_t daddr);

        string convertKeyValue(uint64_t fid);    // Peixuan 04212020

	int RankComputation(QueueDiscItem* item, Flow_pl* currFlow); 

	vector<int> weights;
	vector<uint32_t> flowsize;

	

    public:

        int m_one;

        int flowNo;

        /**

        * \brief Get the type ID.

        * \return the object TypeId

        */

        static TypeId GetTypeId(void);

        /**

        * \brief AFQ constructor

        *

        * Creates a queue with a default or customed number of packets

        */

        AFQ();

        explicit AFQ(int);

        virtual ~AFQ();

        bool DoEnqueue(Ptr<QueueDiscItem> item);

	bool FifoEnqueue(QueueDiscItem* item, int index, int level);

        Ptr<QueueDiscItem> DoDequeue(void);

 	Ptr<QueueDiscItem> FifoDequeue(int);

        Ptr<const QueueDiscItem> DoPeek(void) const;

        bool CheckConfig(void);

        void InitializeParams(void);

        uint32_t m_limit;    //!< Maximum number of packets that can be stored

        Ptr<QueueDiscItem> DoRemove(void);

        void setCurrentRound(int);

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

	void Record(string fname, int departureRound, Ptr<QueueDiscItem> item);

	void tagRecord(int flowiid, int uid, int departureRound, Ptr<QueueDiscItem> item);

	void PacketRecord(string fname, int departureRound, Ptr<QueueDiscItem> item);

	void Rate(int enqueCount, int dequeCount, int overflowCount, int reloadCount, int migrationCount);

	void RecordFlow(string flowlabel, int departureRound,int uid, string filename);

	bool LevelEnqueue(QueueDiscItem* item, int flowid, int departureRound,int uid);

	void AddTag(int flowid, int departureRound, int uid, QueueDiscItem* item);

	void RecordFCT(string filename, int index,double FCT);


    };



}
