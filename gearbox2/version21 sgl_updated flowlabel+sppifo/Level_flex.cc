#include "ns3/object-factory.h"
#include "ns3/Level_flex.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/pfifo-fast-queue-disc.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/Replace_string.h"
#include "ns3/simulator.h"


namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("Level_flex");
    NS_OBJECT_ENSURE_REGISTERED(Level_flex);

    TypeId Level_flex::GetTypeId(void)
    {
      static TypeId tid = TypeId("ns3::Level_flex")
            .SetParent<Object>()
            .SetGroupName("TrafficControl")
            .AddConstructor<Level_flex>()
            ;
        return tid;
    }

    Level_flex::Level_flex() : Level_flex(DEFAULT_VOLUME, 0) {
        NS_LOG_FUNCTION(this);
    }

    Level_flex::Level_flex(int vol, int index) {
        NS_LOG_FUNCTION(this);
        this->volume = vol;
        this->currentIndex = index;
        this->pifo = Pifo(H_value, L_value);

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


    Level_flex::~Level_flex() {
        NS_LOG_FUNCTION(this);
    }

    QueueDiscItem* Level_flex::fifoEnque(QueueDiscItem* item, int index) {
        NS_LOG_FUNCTION(this);

	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
		//for printing the departure round
		Packet* packet = GetPointer(item->GetPacket());
		GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		return item;
	}

	//modify the fifoenque tag
        GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	fifoenque = fifoenque + 1;
        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));
	tag.SetFifoenque(tag.GetFifoenque() + 1);
	item->GetPacket()->ReplacePacketTag(tag);

        pkt_cnt++;

	return NULL;
    }



    // return the pkt being pushed out in this level
    QueueDiscItem* Level_flex::pifoEnque(QueueDiscItem* item) {
        NS_LOG_FUNCTION(this);

        // get the tag of item
        GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
        int departureRound = tag.GetDepartureRound();
       
 	// enque into pifo, if havePifo && ( < maxValue || < L)
	if (departureRound < getPifoMaxValue() || isAllFifosEmpty()) {
            item = pifo.Push(item, departureRound);
	}

	return item;
    }


    QueueDiscItem* Level_flex::fifoDeque(int index) {//for reload
        if (isSelectedFifoEmpty(index)) {
            return 0;
        }

        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());
	fifodeque = fifodeque + 1;

	//modify the fifodeque tag
	//fifodeque+1
	GearboxPktTag tag;
        GetPointer(item)->GetPacket()->PeekPacketTag(tag);
	tag.SetFifodeque(tag.GetFifodeque() + 1);
	item->GetPacket()->ReplacePacketTag(tag);	

        pkt_cnt--;

        return GetPointer(item);
    }



    QueueDiscItem* Level_flex::pifoDeque() {
        NS_LOG_FUNCTION(this);

        Ptr<QueueDiscItem> item = pifo.Pop();
	GearboxPktTag tag;
	GetPointer(item)->GetPacket()->PeekPacketTag(tag);

	int departureround = tag.GetDepartureRound();	    
	setCurrentIndex(cal_index(departureround));

        if(pifo.Size()<pifo.LowestSize() && reload_hold == 0){//!!!!must go through all the packets
		InitializeReload();
	}

        return GetPointer(item);
    }


    bool Level_flex::ifLowerthanL(){
	if(pifo.Size()<pifo.LowestSize()){
		return true;
	}
	else{
		return false;
	}

    }

    void Level_flex::InitializeReload() {

	int earliestFifo = getEarliestFifo();

	if (earliestFifo != -1) {
		reload_hold = 1;					
		reload_fifoSize = getFifoNPackets(getEarliestFifo());
		reload_size = 0;
		pifo.UpdateMaxValid(0); // pifo will not update maxtag automatically
		reloadIni = reloadIni + 1;
	}

    }

    void Level_flex::TerminateReload(){
	reload_hold = 0;
	// the reload process is over, reset the flag and set the maxvalue to be the real (all pkts in the pifo) maxvalue
	pifo.UpdateMaxValid(1);
	pifo.UpdateMaxValue();	
    }






   // with speed up K
   int Level_flex::Reload(int k) {
	int npkts;

	// if no pkt in fifos
        int earliestFifo = getEarliestFifo();

        if (earliestFifo == -1) {
        	return 0;
	}

	npkts = k < getFifoNPackets(earliestFifo) ? k : getFifoNPackets(earliestFifo);
	remainingQ = k - npkts;

	// if start a new reloading process, set the pifomaxtag to be the max valid value of the range of the reloading fifo 
	bool defaultMaxFlag = 0;
	if (reload_size == 0){
		defaultMaxFlag = 1;
	}

	for (int i = 0; i < npkts; i++){ 
		k--;       
		reload = reload + 1;

		// load the reloaded pkt into pifo, the fifoDeque return pkt will not cause fifo overflow
		QueueDiscItem* re = fifoDeque(earliestFifo);
		GearboxPktTag tag0;
		Packet* packet = GetPointer(re->GetPacket());
		packet->PeekPacketTag(tag0);
		
		if (defaultMaxFlag == 1){
			setPifoMaxValue((tag0.GetDepartureRound() / GRANULARITY) * GRANULARITY + GRANULARITY - 1);
			defaultMaxFlag = 0;
		}
		
		//20220103 only when the pkt's tag <= pifomax, the pkt can be enqueued to pifo
		if (tag0.GetDepartureRound() <= getPifoMaxValue()){
            		QueueDiscItem* re1 = pifo.Push(re, tag0.GetDepartureRound());

			if (re1 != NULL){
				GearboxPktTag tag;
				re1->GetPacket()->PeekPacketTag(tag);
				int departureround = tag.GetDepartureRound();	    
			
		    		fifoEnque(re1, cal_index(departureround));
				// reset the max tag value of pifo when the returned pkt's rank is smaller than the pifomaxvalue
				
				if (departureround < getPifoMaxValue()){
					setPifoMaxValue(departureround);
				}
			}
		}

        }
	return npkts;

    }


    int Level_flex::getEarliestFifo() {//won't find the current index

        for (int i = 0; i < DEFAULT_VOLUME; i++) {
            if (!fifos[(i + currentIndex) % DEFAULT_VOLUME]->IsEmpty()) {//start from currentindex+1 (if with migration) or from earliestfifo+1?
                return (i + currentIndex) % DEFAULT_VOLUME;
            }
        }

        return -1; // return -1 if all fifos are empty
    }


    QueueDiscItem* Level_flex::pifopeek() {
	return pifo.Peek();
    }


    void Level_flex::setCurrentIndex(int index) {
        currentIndex = index;
    }


    void Level_flex::setRemainingQ(int remaining) {
        remainingQ = remaining;
    }


    void Level_flex::getAndIncrementIndex() {
        if (currentIndex + 1 < volume) {
            currentIndex++;
        }
        else {
     	     currentIndex = 0;
        }
    }


    bool Level_flex::isCurrentFifoEmpty() {
        return fifos[currentIndex]->IsEmpty();
    }


    bool Level_flex::isSelectedFifoEmpty(int index) {
        return fifos[index]->IsEmpty();
    }


    bool Level_flex::isAllFifosEmpty(){
	for (int i=0;i<DEFAULT_VOLUME;i++){
		if(!isSelectedFifoEmpty(i)){
			return false;
		}
	}
	return true;
    }


    int Level_flex::getCurrentIndex() {
        return currentIndex;
    }


    int Level_flex::getCurrentFifoSize() {
        return fifos[currentIndex]->GetNBytes();
    }


    int Level_flex::getCurrentFifoNPackets() {
	return getFifoNPackets(currentIndex);
    }


   int Level_flex::getFifoMaxNPackets(){
	int max = fifos[0]->GetNPackets();
	for (int i = 1; i < FIFO_PER_LEVEL; i++){
		if ((unsigned)max < fifos[i]->GetNPackets()){
			max = fifos[i]->GetNPackets();
		}
	}
	return max;
   }


    int Level_flex::getFifoNPackets(int index) {
	return fifos[index]->GetNPackets();
    }


    int Level_flex::size() {
        return sizeof(fifos) / sizeof(fifos[0]);
    }


    int Level_flex::getPifoMaxValue() {
        return pifo.GetMaxValue();
    }


    void Level_flex::setPifoMaxValue(int max) {
        pifo.SetMaxValue(max);
    }


    int Level_flex::getRemainingQ() {
        return remainingQ;
    }


    void Level_flex::pifoPrint(){
	pifo.Print();
    }


    void Level_flex::fifoPrint(int index){
	cout << "fifo "<<index<<" Print:";
	for (int i=0;i<getFifoNPackets(index);i++)
  	{	
		Ptr<QueueItem> item = fifos[index]->Dequeue();
    		Packet* packet = GetPointer(item->GetPacket());
	 	GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		cout << "(" << i << ": " << tag.GetDepartureRound() << ", " << tag.GetUid() << ", "<<cal_index(tag.GetDepartureRound())<<") ";
		fifos[index]->Enqueue(item);
  	}
	cout << endl;
    }


    int Level_flex::getFifoTotalNPackets(){
	int cnt = 0; 
	for (int i = 0; i < DEFAULT_VOLUME; i++){  
		cnt += getFifoNPackets(i); 
	} 

	return cnt;    
    }


    int Level_flex::getReloadFifoSize(){
	return reload_fifoSize;
    }


    int Level_flex::getPifoSize(){
	return pifo.Size();
    }


    int Level_flex::getReloadSize(){
	return reload_size;
    }


    void Level_flex::updateReloadSize(int size) {
        reload_size = reload_size + size;
    }


    int Level_flex::getReloadHold(){
	return reload_hold;
    }


    void Level_flex::setReloadHold(int hold){
	reload_hold =hold;
    }


    void Level_flex::setPreviousIndex(int idx){
	previous_idx =idx;
    }


    int Level_flex::getPreviousIndex(){
	return previous_idx;
    }


    bool Level_flex::finishCurrentFifoReload(){

	if(reload_size >= reload_fifoSize){
		return true;
	}
	else{
		return false;
	}
    }


    void Level_flex::SetLevel(int le){
	level = le;
    }


    int Level_flex::cal_index(int departureRound){
	int term = 1;
	for (int i = 0; i < 2; i++){
		term *= FIFO_PER_LEVEL;
	}
	return departureRound / term % FIFO_PER_LEVEL;
    }


    int Level_flex::getPifoEnque(){
	return pifo.GetPifoEnque();
    }

    int Level_flex::getPifoDeque(){
	return pifo.GetPifoDeque();
    }

    int Level_flex::getFifoEnque(){
	return fifoenque;
    }

    int Level_flex::getFifoDeque(){
	return fifodeque;
    }

    int Level_flex::getReload(){
	return reload;
    }

    int Level_flex::getReloadIni(){
	return reloadIni;
    }

}





