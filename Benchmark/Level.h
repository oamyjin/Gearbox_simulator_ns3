//
// Created by Zhou Yitao on 2018-12-04.

#ifndef HIERARCHICALSCHEDULING_HIERARCHY_H
#define HIERARCHICALSCHEDULING_HIERARCHY_H

#include <queue>
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/address.h"
#include "ns3/drop-tail-queue.h"

using namespace std;
namespace ns3 {

class Level : public Object{
private:
    static const int DEFAULT_VOLUME = 10;
    int volume;                         // num of fifos in one level
    int currentIndex;                   // current serve index
    Queue *fifos[10];
public:
    static TypeId GetTypeId (void);
    Level();
    Level(int, int);
    ~Level();
    void enque(QueueDiscItem* packet, int index);
    QueueDiscItem* deque();
    int getCurrentIndex();
    void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
    void getAndIncrementIndex();
    int getCurrentFifoSize();
    bool isCurrentFifoEmpty();
};
}


#endif //HIERARCHICALSCHEDULING_HIERARCHY_H
