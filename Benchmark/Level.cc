//
// Created by Zhou Yitao on 2018-12-04.
//

#include "ns3/object-factory.h"
#include "ns3/Level.h"
#include "ns3/ptr.h"
#include "ns3/log.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("Level");
NS_OBJECT_ENSURE_REGISTERED (Level);

TypeId Level::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Level")
    .SetParent<Object> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<Level> ()
  ;
  return tid;
}

Level::Level() : Level(10, 0){
    NS_LOG_FUNCTION (this);
}

Level::Level(int vol, int index){
    this->volume = vol;
    this->currentIndex = index;

    // Debug: Peixuan 07062019 Level Constructor
    ObjectFactory factory;
    factory.SetTypeId ("ns3::DropTailQueue");
    factory.Set ("Mode", EnumValue (Queue::QUEUE_MODE_PACKETS));
    factory.Set ("MaxPackets", UintegerValue (volume));

    for (int i=0; i<volume; i++) {
        fifos[i] = GetPointer(factory.Create<Queue> ());
	uint32_t maxSize = 1000;
	fifos[i]->SetMaxPackets(maxSize);
    }
    //fprintf(stderr, "Constructed Level\n"); // Debug: Peixuan 07062019

}

Level::~Level(){
    NS_LOG_FUNCTION (this);
}


void Level::enque(QueueDiscItem* item, int index) {
    // packet.setInsertFifo(index);
    // packet.setFifoPosition(static_cast<int>(fifos[index].size()));
    // hdr_ip* iph = hdr_ip::access(packet);
    //cout << "index:" << index << " fifos[index]->GetNBytes():" << fifos[index]->GetNBytes() << " item:" << item << endl;
    fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));
    //cout << "index:" << index << " fifos[index]->GetNBytes():" << fifos[index]->GetNBytes() << " item:" << item << endl;
}

QueueDiscItem* Level::deque() {

    //fprintf(stderr, "Dequeue from this level\n"); // Debug: Peixuan 07062019

    //cout << "currentIndex:" << currentIndex << " fifos[currentIndex]->GetNBytes():" << fifos[currentIndex]->GetNBytes() << endl;
    //if (isCurrentFifoEmpty()) {
    if (fifos[currentIndex]->GetNBytes() == 0){
	//fprintf(stderr, "No packet in the current serving FIFO\n"); // Debug: Peixuan 07062019
        return 0;
    }
    Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (fifos[currentIndex]->Dequeue());
    return GetPointer(item);
}

int Level::getCurrentIndex() {
    return currentIndex;
}

void Level::setCurrentIndex(int index) {
    currentIndex = index;
}

void Level::getAndIncrementIndex() {
    if (currentIndex + 1 < volume) {
        currentIndex++;
    } else {
        currentIndex = 0;
    }
}

bool Level::isCurrentFifoEmpty() {
    //fprintf(stderr, "Checking if the FIFO is empty\n"); // Debug: Peixuan 07062019
    //fifos[currentIndex]->length() == 0;
    //fprintf(stderr, "Bug here solved\n"); // Debug: Peixuan 07062019
    //cout << "==isCurrentFifoEmpty fifos[currentIndex]->GgetNBytes():" << fifos[currentIndex]->GetNBytes() << " currentIndex:" << currentIndex << endl;
    return fifos[currentIndex]->IsEmpty() == 0;
}

int Level::getCurrentFifoSize() {
    return fifos[currentIndex]->GetNBytes();
}
}
