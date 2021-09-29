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



using namespace std;

namespace ns3 {

    class Gearbox_pl_fid_flex : public QueueDisc {



    private:



        static const int DEFAULT_VOLUME = 3;



        static const int FIFO_PER_LEVEL = 8;         // 01212020 Peixuan flex level
        static const int DEFAULT_WEIGHT = 2;         // 01032019 Peixuan default weight
        static const int DEFAULT_BRUSTNESS = 1000;    // 01032019 Peixuan default brustness
        static const int TIMEUNIT = 1;    // 01032019 Peixuan default brustness
 	static const int SPEEDUP_FACTOR = 5;



        int volume;                     // num of Level_flexs in scheduler
        int currentRound;           // current Round
        int pktCount;           // packet count
        int enqueCount;
        int dequeCount;
        int dropCount;

        Level_flex levels[3];
        Level_flex levelsB[2];       // Back up Levels


        //Level_flex forthLevel;

        //Level_flex thirdLevel;


        Level_flex hundredLevel;
        Level_flex decadeLevel;


        //Level_flex thirdLevelB;    // Back up Level_flexs



        //Level_flex hundredLevelB;    // Back up Level_flexs



        Level_flex decadeLevelB;     // Back up Levels
        bool level0ServingB;          // is serve Back up Levels
        bool level1ServingB;          // is serve Back up Levels


        //bool level2ServingB;          // is serve Back up Levels



        //bool level3ServingB;          // is serve Back up Levels



        //bool level4ServingB;          // is serve Back up Levels







        //vector<Flow_pl> flows;



        vector<Ptr<QueueDiscItem>> pktCurRound;



        vector<Ptr<QueueDiscItem>> runRound();



        vector<Ptr<QueueDiscItem>> serveUpperLevel(int);



        void setPktCount(int);







        //12132019 Peixuan



        //typedef std::map<int, Flow_pl*> FlowTable;

        typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;



        //typedef std::map<int, Flow_pl*> TestIntMap;
        //TestIntMap testIntMap;
        //12132019 Peixuan
        //Flow_pl* getFlowPtr(nsaddr_t saddr, nsaddr_t daddr);
        Flow_pl* getFlowPtr(uint64_t fid);   // Peixuan 04212020
        //int getFlowPtr(ns_addr_t saddr, ns_addr_t daddr);
        //Flow_pl* insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness);
        Flow_pl* insertNewFlowPtr(uint64_t fid, int weight, int brustness);   // Peixuan 04212020
        //int updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr);
        int updateFlowPtr(uint64_t fid, Flow_pl* flowPtr);    // Peixuan 04212020
        //string convertKeyValue(nsaddr_t saddr, nsaddr_t daddr);
        string convertKeyValue(uint64_t fid);    // Peixuan 04212020

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



    };



}
