#ifndef LEVEL_FLEX_H



#define LEVEL_FLEX_H



#include <queue>



#include "ns3/queue.h"



#include "ns3/queue-disc.h"



#include "ns3/address.h"



#include "ns3/drop-tail-queue.h"



#include "ns3/pifo.h"







using namespace std;



namespace ns3 {



 class Level_flex : public Object {



 private:



     static const int DEFAULT_VOLUME = 10; //number of fifos



     static const int FIFO_PER_LEVEL = 10;



     static const int DEFAULT_FIFO_N_SIZE = 20;



     static const int SPEEDUP_FACTOR = 2;

     static const int GRANULARITY = 100;



     int volume;                         // num of fifos in one level



     int currentIndex;                   // current serve index



     int pkt_cnt;                        // 01132021 Peixuan debug



     Queue* fifos[DEFAULT_VOLUME];



     // Map < key: departure round, value: QUeueDiscItem* >



 //priority_queue <Map<int, QueueDiscItem*>> pifos;



     Pifo pifo;



     uint32_t m_limit;    //!< Maximum number of packets that can be stored 



     int H_value =15;    //high threshold of pifo

     int L_value =10;    //low threshold of pifo



     int reload_hold = 0; //if reload_hold = 1 then reload in any case

     int level = 0;

     // record operation count
     int reload = 0;
     int reloadIni = 0;
     int fifoenque = 0;
     int fifodeque = 0;
 

     int reload_size = 0;// the size of packets which already have been reloaded

     int remainingQ = SPEEDUP_FACTOR;

     int previous_idx=0;



 public:



	int reload_fifoSize = 0;//record the reload corresponding fifo size

     static TypeId GetTypeId(void);



     Level_flex();



     Level_flex(int vol, int index);



     ~Level_flex();



     //gearbox2



     //bool enque(QueueDiscItem* item, int index, bool isPifoEnque);   //interface exposed to Gearbox, deciding to call fifoEnque or pifoEnque according to 'isPifoEnque'



	// return queueItem if overflow, otherwise NULL



     QueueDiscItem*  pifoEnque(QueueDiscItem* item);



     QueueDiscItem*  fifoEnque(QueueDiscItem* item, int index);



     QueueDiscItem* pifoDeque();



     QueueDiscItem* fifoDeque(int index);



     const QueueItem* fifopeek1();

	const QueueItem* fifopeek2(int fifo);



     QueueDiscItem* pifopeek();



     // reload



     int Reload(int k);//k  is remainingQ



     int getEarliestFifo(void); //-1 if fifos are all empty, else the earliest fifo index



     int getCurrentIndex();



     void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)



     void getAndIncrementIndex();



     int getCurrentFifoSize();



	int getFifoMaxNPackets();



	int getCurrentFifoNPackets();



	int getFifoNPackets(int index);



     bool isCurrentFifoEmpty();



	bool isSelectedFifoEmpty(int index);



	bool isAllFifosEmpty();



     int size();



     int get_level_pkt_cnt();            // 01132021 Peixuan debug



     int getPifoMaxValue(void); // the max tag in the pifo
     void setPifoMaxValue(int);



	void pifoPrint();



	int getFifoTotalNPackets();



	int getPifoSize();

	void InitializeReload();

	void TerminateReload();

	int getReloadHold();

	void setReloadHold(int hold);

	int getReloadSize();

	void updateReloadSize(int size);

 	bool finishCurrentFifoReload();

	bool ifLowerthanL();

	void setRemainingQ(int remaining);

	int getRemainingQ();

	int getReloadFifoSize();

	void fifoPrint(int index);

	void SetLevel(int le);

	int cal_index(int departureRound);

	void setPreviousIndex(int idx);

	int getPreviousIndex();

	int getPifoEnque();
	int getPifoDeque();
	int getFifoEnque();
	int getFifoDeque();
	int getReload();
	int getReloadIni();



 };



}



#endif //LEVEL_FLEX_H
