
#include "ns3/object-factory.h"
#include "ns3/Level_flex.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/log.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("Level_flex");

NS_OBJECT_ENSURE_REGISTERED (Level_flex);

TypeId Level_flex::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Level_flex")
    .SetParent<Object> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<Level_flex> ()
  ;
  return tid;
}

Level_flex::Level_flex(): Level_flex(DEFAULT_VOLUME, 0){
    NS_LOG_FUNCTION (this);
}

Level_flex::Level_flex(int vol, int index) {
    NS_LOG_FUNCTION (this);
    this->volume = vol;
    this->currentIndex = index;
    
    ObjectFactory factory;
    factory.SetTypeId ("ns3::DropTailQueue");
    factory.Set ("Mode", EnumValue (Queue::QUEUE_MODE_PACKETS));
    factory.Set ("MaxPackets", UintegerValue (2));

    for (int i=0; i<volume; i++) {
        fifos[i] = GetPointer(factory.Create<Queue> ());
	uint32_t maxSize = 100;
	fifos[i]->SetMaxPackets(maxSize);
    }
}

Level_flex::~Level_flex(){
    NS_LOG_FUNCTION (this);
}

void Level_flex::enque(QueueDiscItem* item, int index) {
    // packet.setInsertFifo(index);
    // packet.setFifoPosition(static_cast<int>(fifos[index].size()));
    // hdr_ip* iph = hdr_ip::access(packet);
    NS_LOG_FUNCTION (this);
    cout << "E===getCurrentFifoSize:" << getCurrentFifoSize() << " getCurrentIndex:" << getCurrentIndex() << endl;
    fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));
    pkt_cnt++;
    cout << "E===getCurrentFifoSize:" << getCurrentFifoSize() << " getCurrentIndex:" << getCurrentIndex() << endl;
}

QueueDiscItem* Level_flex::deque() {

    //fprintf(stderr, "Dequeue from this Level_flex\n"); // Debug: Peixuan 07062019

    if (isCurrentFifoEmpty()) {
        //fprintf(stderr, "No packet in the current serving FIFO\n"); // Debug: Peixuan 07062019
        return 0;
    }
    cout << "D==getCurrentFifoSize:" << getCurrentFifoSize() << " getCurrentIndex:" << getCurrentIndex() << endl;
    Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (fifos[currentIndex]->Dequeue());
    pkt_cnt--;
    cout << "D==getCurrentFifoSize:" << getCurrentFifoSize() << " getCurrentIndex:" << getCurrentIndex() << endl;
    return GetPointer(item);
}

int Level_flex::getCurrentIndex() {
    return currentIndex;
}

void Level_flex::setCurrentIndex(int index) {
    currentIndex = index;
}

void Level_flex::getAndIncrementIndex() {
    if (currentIndex + 1 < volume) {
        currentIndex++;
    } else {
        currentIndex = 0;
    }
}

bool Level_flex::isCurrentFifoEmpty() {
    //fprintf(stderr, "Checking if the FIFO is empty\n"); // Debug: Peixuan 07062019
    //fifos[currentIndex]->length() == 0;
    //fprintf(stderr, "Bug here solved\n"); // Debug: Peixuan 07062019
    return fifos[currentIndex]->IsEmpty();
}

int Level_flex::getCurrentFifoSize() {
    return fifos[currentIndex]->GetNBytes();
}

int Level_flex::size() {
    // get fifo number
    //return fifos.size();
    return sizeof(fifos)/sizeof(fifos[0]);
}

int Level_flex::get_level_pkt_cnt() {
    // get pkt_cnt
    return pkt_cnt;
}
}
