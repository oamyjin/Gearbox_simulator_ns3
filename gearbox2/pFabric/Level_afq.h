#ifndef Level_afq_H
#define Level_afq_H
#include <queue>
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/address.h"
#include "ns3/drop-tail-queue.h"
using namespace std;
namespace ns3 {
 class Level_afq : public Object {
 private:
     static const int DEFAULT_VOLUME = 1;//1000//1//20000
     static const int FIFO_PER_LEVEL = 1;//1000//1//20000
     static const int DEFAULT_FIFO_N_SIZE = 500;//20//20000
     int volume;                         // num of fifos in one level
     int currentIndex;                   // current serve index
     int pkt_cnt;                        // 01132021 Peixuan debug
     Queue* fifos[DEFAULT_VOLUME];
     int earliestFifo = 0;
     int fifoenque, fifodeque = 0;
     // Map < key: departure round, value: QUeueDiscItem* >
     //priority_queue <Map<int, QueueDiscItem*>> pifos;
     uint32_t m_limit;    //!< Maximum number of packets that can be stored 
     int level = 0;
     int previous_idx=0;
 public:
     static TypeId GetTypeId(void);
     Level_afq();
     Level_afq(int vol, int index);
     ~Level_afq();
     //gearbox2
     //bool enque(QueueDiscItem* item, int index, bool isPifoEnque);   //interface exposed to Gearbox, deciding to call fifoEnque or pifoEnque according to 'isPifoEnque'
	// return queueItem if overflow, otherwise NULL
     QueueDiscItem*  fifoEnque(QueueDiscItem* item, int index, int flowid);
     QueueDiscItem* fifoDeque(int index);
     const QueueItem* fifopeek1();
     int getEarliestFifo(void); //-1 if fifos are all empty, else the earliest fifo index
     int getCurrentIndex();
     void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
     int getCurrentFifoSize();
	
	int getCurrentFifoNPackets();
	int getFifoNPackets(int index);
     bool isCurrentFifoEmpty();
	bool isSelectedFifoEmpty(int index);
	bool isAllFifosEmpty();
     int size();
     int get_level_pkt_cnt();            // 01132021 Peixuan debug
	int getFifoTotalNPackets();
	void fifoPrint(int index);
	void SetLevel(int le);
	int cal_index(int departureRound);
	void setPreviousIndex(int idx);
	int getPreviousIndex();
	int getFifoEnque();
	int getFifoDeque();
	int getFifoMaxNPackets();
 };
}
#endif //Level_afq_H
