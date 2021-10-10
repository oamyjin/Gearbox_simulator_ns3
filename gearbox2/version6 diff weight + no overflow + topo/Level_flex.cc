#include "ns3/object-factory.h"

#include "ns3/Level_flex.h"

#include "ns3/packet.h"

#include "ns3/ptr.h"

#include "ns3/log.h"

#include "ns3/pfifo-fast-queue-disc.h"

#include "ns3/queue.h"

#include "ns3/queue-disc.h"



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

        //level 0 or the pifo overflow to fifo

        NS_LOG_FUNCTION(this);
        
	if (! (fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
	        cout << " fifo overflow: fifo_n_pkt_size:"  << fifos[index]->GetNPackets() << " index:" << index << " DEFAULT_FIFO_N_SIZE:" << DEFAULT_FIFO_N_SIZE << endl;
		//Drop(item);
		return item;
	}

        GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));
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
	//cout << "PE= departureRound:" << departureRound << " getPifoMaxValue():" << getPifoMaxValue() << " pifo.Size():" << pifo.Size() << " pifo.LowestSize():" << pifo.LowestSize() << endl;

	int earliestFifo = getEarliestFifo();
	// enque into pifo only if < max_tag or this level's fifos are empty
	cout << "getPifoMaxValue:" << getPifoMaxValue() << endl;
        if (departureRound < getPifoMaxValue() || earliestFifo == -1) {
            QueueDiscItem* re = pifo.Push(item, departureRound);
	    cout << "re:" << re << endl << "enqueued ";
	    pifo.Print();
            if (re != NULL) {
                GearboxPktTag tag1;
                re->GetPacket()->PeekPacketTag(tag1);
	    	cout << "re.tag:" << tag1.GetDepartureRound() << endl;
                re = fifoEnque(re, tag1.GetIndex()); // redirect to this level's fifo
		cout << "re:" << re << endl; // if successfully enqueued to fifo, return 0; if overflow, return the pkt
            }
	    cout << " return re " << re << endl;
	    return re; // return the overflow pkt
        }

        // enque the enqued pkt into fifo     
        else {
	    cout << tag.GetDepartureRound() << " redirect to fifo index:" << tag.GetIndex() << endl;
            return fifoEnque(item, tag.GetIndex());
        }
    }



    QueueDiscItem* Level_flex::fifoDeque(int index) {//for reload
	cout << "isSelectedFifoEmpty(index):" << isSelectedFifoEmpty(index) << " index:" << index << endl;
        if (isSelectedFifoEmpty(index)) {
            return 0;
        }
        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());
	cout << "item:" << item << endl;
        pkt_cnt--;
        return GetPointer(item);
    }



    QueueDiscItem* Level_flex::pifoDeque() {
        NS_LOG_FUNCTION(this);
        Ptr<QueueDiscItem> item = pifo.Pop();
	GearboxPktTag tag;
	GetPointer(item)->GetPacket()->PeekPacketTag(tag);
	setCurrentIndex(tag.GetIndex());
	cout << "ifLowerThanLthenReload pifo.Size():" << pifo.Size() << " L:" << pifo.LowestSize() << " --------------" << endl;	
	if (pifo.Size() < pifo.LowestSize()) {      
		reload();
	}
	cout << "reloaded --------------------------------------------" << endl;
        return GetPointer(item);
    }

   // with speed up K
   void Level_flex::reload() {
	int k = SPEEDUP_FACTOR;
        while (k > 0) {
	    // if no pkt in fifos
            int earliestFifo = getEarliestFifo();
            if (earliestFifo == -1) {
		cout << "earliestFifo:" << earliestFifo << endl;
                break;
            }
            setCurrentIndex(earliestFifo);
	    int npkts = k < getFifoNPackets(earliestFifo) ? k : getFifoNPackets(earliestFifo);
	    for (int i = 0; i < npkts; i++){ 
		k--;       
		cout << SPEEDUP_FACTOR - k << "th reload" << " pifoSize:" << pifo.Size() << endl;
		// load the reloaded pkt into pifo, the fifoDeque return pkt will not cause fifo overflow
		//pifoEnque(fifoDeque(earliestFifo));// don't change the current index
		QueueDiscItem* re = fifoDeque(earliestFifo);
		GearboxPktTag tag0;
		re->GetPacket()->PeekPacketTag(tag0);
            	re = pifo.Push(re, tag0.GetDepartureRound());
		if (re != NULL){
			GearboxPktTag tag;
			re->GetPacket()->PeekPacketTag(tag);
			cout << "redirect re:" << re << " dp:" << tag.GetDepartureRound() << endl;
			fifoEnque(re, tag.GetIndex());
		}
     	    }
        }
    }



    /*
    // without speed up
    void Level_flex::ifLowerThanLthenReload() {
        while (pifo.Size() < pifo.LowestSize()) {
            int earliestFifo = getEarliestFifo();
            if (earliestFifo == -1) {
                break;
            }
            setCurrentIndex(earliestFifo);
	    int npkts = getFifoNPackets(earliestFifo);
	    for (int i = 0; i < npkts; i++){        
		pifoEnque(fifoDeque(earliestFifo));// don't change the current index
     	    }
        }
    }*/
    



    int Level_flex::getEarliestFifo() {
        for (int i = 1; i < DEFAULT_VOLUME; i++) {
            if (!fifos[(i + currentIndex) % DEFAULT_VOLUME]->IsEmpty()) {//start from currentindex+1 (if with migration) or from earliestfifo+1?
                return (i + currentIndex) % DEFAULT_VOLUME;
            }
        }
        return -1; // return -1 if all fifos are empty
    }





    const QueueItem* Level_flex::fifopeek() {
	Ptr<const QueueItem> item;
	if (isCurrentFifoEmpty()) {
	    currentIndex = getEarliestFifo();
	    if (currentIndex == -1){
		currentIndex = 0;
		return NULL;
	    }
	    //cout << " empty currentIndex:" << currentIndex << endl;
	}
	//cout << " currentIndex:" << currentIndex << endl;
	item = fifos[currentIndex]->Peek();
	return GetPointer(item);
    }


    QueueDiscItem* Level_flex::pifopeek() {
        return pifo.Peek();

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

    int Level_flex::getCurrentFifoSize() {
        return fifos[currentIndex]->GetNBytes();
    }

    int Level_flex::getCurrentFifoNPackets() {
	return getFifoNPackets(currentIndex);
    }
 
    int Level_flex::getFifoNPackets(int index) {
	return fifos[index]->GetNPackets();
    }

    int Level_flex::size() {

        return sizeof(fifos) / sizeof(fifos[0]);

    }



    int Level_flex::get_level_pkt_cnt() {
        return pkt_cnt;
    }

    int Level_flex::getPifoMaxValue() {
        return pifo.GetMaxValue();
    }

    int Level_flex::getPifoSize() {
        return pifo.Size();
    }

    void Level_flex::pifoPrint(){
	pifo.Print();
    }
    
    int Level_flex::getFifoTotalNPackets(){
	int cnt = 0;
	for (int i = 0; i < DEFAULT_VOLUME; i++){
		cnt += getFifoNPackets(i);
	}
	return cnt;
    }
}
