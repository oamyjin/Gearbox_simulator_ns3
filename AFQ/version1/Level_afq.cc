#include "ns3/object-factory.h"
#include "ns3/Level_afq.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/pfifo-fast-queue-disc.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/Replace_string.h"
#include "ns3/simulator.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("Level_afq");
    NS_OBJECT_ENSURE_REGISTERED(Level_afq);
    TypeId Level_afq::GetTypeId(void)
    {
      static TypeId tid = TypeId("ns3::Level_afq")
            .SetParent<Object>()
            .SetGroupName("TrafficControl")
            .AddConstructor<Level_afq>()
            ;
        return tid;
    }
    Level_afq::Level_afq() : Level_afq(DEFAULT_VOLUME, 0) {
        NS_LOG_FUNCTION(this);
    }
    Level_afq::Level_afq(int vol, int index) {
        NS_LOG_FUNCTION(this);
        this->volume = vol;
        this->currentIndex = index;
	this->fifoenque = 0;
	this->fifodeque = 0;
        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue");
        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));
        factory.Set("MaxPackets", UintegerValue(DEFAULT_FIFO_N_SIZE));
        for (int i = 0; i < volume; i++) {
            fifos[i] = GetPointer(factory.Create<Queue>());
            uint32_t maxSize = DEFAULT_FIFO_N_SIZE;
            fifos[i]->SetMaxPackets(maxSize);
        }
    }
    Level_afq::~Level_afq() {
        NS_LOG_FUNCTION(this);
    }
    QueueDiscItem* Level_afq::fifoEnque(QueueDiscItem* item, int index, int flowid) {
        NS_LOG_FUNCTION(this);
        
	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
		//for printing the departure round
		Packet* packet = GetPointer(item->GetPacket());
		GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		return item;
	}
        GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	fifoenque += 1;
        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));
        pkt_cnt++;

	return NULL;
    }
    // return the pkt being pushed out in this level


    QueueDiscItem* Level_afq::fifoDeque(int index) {//for reload
        if (isSelectedFifoEmpty(index)) {
            return 0;
        }
        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());
	

	GearboxPktTag tag;
        GetPointer(item)->GetPacket()->PeekPacketTag(tag);
	fifodeque += 1;
        pkt_cnt--;
        return GetPointer(item);
    }

    int Level_afq::getEarliestFifo() {//won't find the current index
        for (int i = 0; i < DEFAULT_VOLUME; i++) {
            if (!fifos[(i + currentIndex) % DEFAULT_VOLUME]->IsEmpty()) {//start from currentindex+1 (if with migration) or from earliestfifo+1?
                return (i + currentIndex) % DEFAULT_VOLUME;
            }
        }
        return -1; // return -1 if all fifos are empty
    }
    const QueueItem* Level_afq::fifopeek1() {
	Ptr<const QueueItem> item;
	if (isCurrentFifoEmpty()) {
	    currentIndex = getEarliestFifo();
	    if (currentIndex == -1){
		currentIndex = 0;
		return NULL;
	    }
	    //////cout << " empty currentIndex:" << currentIndex << endl;
	}
	//////cout << " currentIndex:" << currentIndex << endl;
	item = fifos[currentIndex]->Peek();
	return GetPointer(item);
    }


    void Level_afq::setCurrentIndex(int index) {
        currentIndex = index;
    }


    bool Level_afq::isCurrentFifoEmpty() {
        return fifos[currentIndex]->IsEmpty();
    }
    bool Level_afq::isSelectedFifoEmpty(int index) {
        return fifos[index]->IsEmpty();
    }
    bool Level_afq::isAllFifosEmpty(){//all fifos but not currenindex fifo
	for (int i=0;i<DEFAULT_VOLUME;i++){
		if(!isSelectedFifoEmpty((i + currentIndex) % DEFAULT_VOLUME)){
			return false;
		}
	}
	return true;
    }
    int Level_afq::getCurrentIndex() {
        return currentIndex;
    }

    int Level_afq::getCurrentFifoSize() {
        return fifos[currentIndex]->GetNBytes();
    }
    int Level_afq::getCurrentFifoNPackets() {
	return getFifoNPackets(currentIndex);
    }
 
    int Level_afq::getFifoNPackets(int index) {
	return fifos[index]->GetNPackets();
    }
    int Level_afq::size() {
        return sizeof(fifos) / sizeof(fifos[0]);
    }
    int Level_afq::get_level_pkt_cnt() {
        return pkt_cnt;
    }


    void Level_afq::fifoPrint(int index){
	cout << "fifo "<<index<<" Print:";
	for (int i=0;i<getFifoNPackets(index);i++)
  	{	Ptr<QueueItem> item = fifos[index]->Dequeue();
    		Packet* packet = GetPointer(item->GetPacket());
	 	GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		cout << "(" << i << ": " << tag.GetDepartureRound() << ", " << tag.GetUid() << ", "<<cal_index(tag.GetDepartureRound())<<") ";
		fifos[index]->Enqueue(item);
  	}
	cout << endl;
    }

    int Level_afq::getFifoTotalNPackets(){ 
	int cnt = 0; 
	for (int i = 0; i < DEFAULT_VOLUME; i++){  
		cnt += getFifoNPackets(i); 
	} 
	return cnt;    
    }
	
    void Level_afq::setPreviousIndex(int idx){
	previous_idx =idx;
    }
  int Level_afq::getPreviousIndex(){
	return previous_idx;
    }
   

	
    void Level_afq::SetLevel(int le){
		level = le;
	}
    int Level_afq::cal_index(int departureRound){
	int term = 1;
	
	for (int i = 0; i < level; i++){
 
		term *= FIFO_PER_LEVEL;
		
	}
	return departureRound / term % FIFO_PER_LEVEL;
    }

  int Level_afq::getFifoMaxNPackets(){
 	int max = fifos[0]->GetNPackets();
 	for (int i = 1; i < FIFO_PER_LEVEL; i++){
  		if ((unsigned)max < fifos[i]->GetNPackets()){
   			max = fifos[i]->GetNPackets();
  		}
 	}
 	return max;
   }

   int Level_afq::getFifoEnque() {        
	return fifoenque;
   }
   int Level_afq::getFifoDeque() {        
	return fifodeque;
   }

}

