#ifndef HIERARCHICALSCHEDULING_HIERARCHY_H
#define HIERARCHICALSCHEDULING_HIERARCHY_H

#include <queue>
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/address.h"
#include "ns3/drop-tail-queue.h"

using namespace std;
namespace ns3 {

class Level_flex : public Object{
private:
    static const int DEFAULT_VOLUME = 8;
    int volume;                         // num of fifos in one level
    int currentIndex;                   // current serve index
    int pkt_cnt;                        // 01132021 Peixuan debug   
    
    Queue *fifos[DEFAULT_VOLUME];
    
    uint32_t m_limit;    //!< Maximum number of packets that can be stored 


public:
    static TypeId GetTypeId (void);
    Level_flex();
    Level_flex(int vol, int index);
    ~Level_flex();
    void enque(QueueDiscItem* item, int index);
    QueueDiscItem* deque();
    int getCurrentIndex();
    void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
    void getAndIncrementIndex();
    int getCurrentFifoSize();
    bool isCurrentFifoEmpty();
    int size();
    int get_level_pkt_cnt();            // 01132021 Peixuan debug
};
}

#endif //HIERARCHICALSCHEDULING_HIERARCHY_H

