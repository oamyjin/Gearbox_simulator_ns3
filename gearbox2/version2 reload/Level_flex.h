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

        static const int DEFAULT_VOLUME = 8;

        static const int FIFO_PER_LEVEL = 8;

        static const int DEFAULT_FIFO_N_SIZE = 10;

        int volume;                         // num of fifos in one level

        int currentIndex;                   // current serve index

        int pkt_cnt;                        // 01132021 Peixuan debug



        Queue* fifos[DEFAULT_VOLUME];

        // Map < key: departure round, value: QUeueDiscItem* >

    //priority_queue <Map<int, QueueDiscItem*>> pifos;

        Pifo pifo;



        uint32_t m_limit;    //!< Maximum number of packets that can be stored 

        int H_value = 20;    //high threshold of pifo

        int L_value = 10;    //low threshold of pifo



    public:

        static TypeId GetTypeId(void);

        Level_flex();

        Level_flex(int vol, int index);

        ~Level_flex();

        //gearbox2

        void enque(QueueDiscItem* item, int index, bool isPifoEnque);   //interface exposed to Gearbox, deciding to call fifoEnque or pifoEnque according to 'isPifoEnque'

        void enque(QueueDiscItem* item, int index);

        void fifoEnque(QueueDiscItem* item, int index);

        QueueDiscItem* fifoDeque();

        void pifoEnque(QueueDiscItem* item);

        QueueDiscItem* pifoDeque();

        const QueueItem* fifopeek();

        QueueDiscItem* pifopeek();

        // reload

        void ifLowerThanLthenReload(void);

        int getEarliestFifo(void); //-1 if fifos are all empty, else the earliest fifo index



        int getCurrentIndex();

        void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)

        void getAndIncrementIndex();

        int getCurrentFifoSize();
	
	int getCurrentFifoNPackets();

        bool isCurrentFifoEmpty();

        int size();

        int get_level_pkt_cnt();            // 01132021 Peixuan debug



        int getPifoMaxValue(void);

        int getFifoTopValue(void);

    };

}



#endif //LEVEL_FLEX_H
