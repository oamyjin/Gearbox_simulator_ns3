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





    //gearbox2



    void Level_flex::enque(QueueDiscItem* item, int index) {

        this->pifoEnque(item);

    }







    void Level_flex::enque(QueueDiscItem* item, int index, bool isPifoEnque) {

        GearboxPktTag tag1;

        item->GetPacket()->PeekPacketTag(tag1);

        GearboxPktTag tag2;

        item->GetPacket()->PeekPacketTag(tag2);

        if (isPifoEnque == 1) {
            this->pifoEnque(item);
        }

        else {
	    cout << "fifoenque index:" << index << " NPkt:" << fifos[index]->GetNPackets() << endl; 
            this->fifoEnque(item, index);

        }

    }



    void Level_flex::fifoEnque(QueueDiscItem* item, int index) {

        //level 0 or the pifo overflow to fifo

        NS_LOG_FUNCTION(this);
	cout << "fifo index:" << index << " Npkt:" << fifos[index]->GetNPackets() << endl;
        
	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
	        cout << " DROP!!! fifo overflow: fifo_n_pkt_size:"  << fifos[index]->GetNPackets()  << " DEFAULT_FIFO_N_SIZE:" << DEFAULT_FIFO_N_SIZE << endl;
		//Drop(item);
		return;
	}

        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));

        pkt_cnt++;

    }



    //gearbox2

    void Level_flex::pifoEnque(QueueDiscItem* item) {

        NS_LOG_FUNCTION(this);

        // get the tag of item

        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

        int departureRound = tag.GetDepartureRound();

        // enque into pifo, if havePifo && ( < maxValue || < L)
	cout << "PE= departureRound:" << departureRound << " getPifoMaxValue():" << getPifoMaxValue() << " pifo.Size():" << pifo.Size() << " pifo.LowestSize():" << pifo.LowestSize() << endl;

        if (departureRound < getPifoMaxValue() || pifo.Size() < pifo.LowestSize()) {

            vector<QueueDiscItem*> re = pifo.Push(item, departureRound);
	    cout << " re.size():" << re.size() << endl;
	    pifo.Print();
            while (re.size() != 0) {

                QueueDiscItem* reItem = re.back();

                GearboxPktTag tag1;

                reItem->GetPacket()->PeekPacketTag(tag1);

                fifoEnque(reItem, tag1.GetIndex()); // get the last item

                re.pop_back(); // delete the last item

            }

        }

        // enque into fifo     
        else {
            fifoEnque(item, tag.GetIndex());

        }
    }





    QueueDiscItem* Level_flex::fifoDeque(int index) {//for reload
	cout << "fifodeque index:" << index << endl;

        if (isSelectedFifoEmpty(index)) {
            return 0;

        }

        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());
        pkt_cnt--;

        return GetPointer(item);

    }



    QueueDiscItem* Level_flex::pifoDeque() {

        NS_LOG_FUNCTION(this);

        Ptr<QueueDiscItem> item = pifo.Pop();
	GearboxPktTag tag;
	GetPointer(item)->GetPacket()->PeekPacketTag(tag);
	cout << " pifodeque departureRound:" << tag.GetDepartureRound() << endl;

        ifLowerThanLthenReload();

        return GetPointer(item);

    }





    void Level_flex::ifLowerThanLthenReload() {
	int k = 0;
        while (pifo.Size() < pifo.LowestSize() && k < SPEEDUP_FACTOR) {
            int earliestFifo = getEarliestFifo();
            if (earliestFifo == -1) {
                break;
            }
            //setCurrentIndex(earliestFifo);

	    for (int i = 0; i < getCurrentFifoNPackets() ; i++){
		cout << "ifLowReload i=" << i << endl;          
		pifoEnque(fifoDeque(earliestFifo));// don't change the current index
		k++;
     	    }
        }

    }



    int Level_flex::getEarliestFifo() {
        for (int i = 1; i < volume; i++) {
            if (!fifos[(i + currentIndex) % 8]->IsEmpty()) {//start from currentindex+1? or from earliestfifo+1?
                return (i + currentIndex) % 8;
            }
        }
        return -1; // return -1 if all fifos are empty
    }





    const QueueItem* Level_flex::fifopeek() {
        //currentIndex = getEarliestFifo();
        /*if (currentIndex == -1){
	    currentIndex = 0; // start to serve next level's fifos
            return 0;
	}*/
	int earliestFifo;
	Ptr<const QueueItem> item;
	if (isCurrentFifoEmpty()) {
	    earliestFifo = getEarliestFifo();
	    item = fifos[earliestFifo]->Peek();
            return GetPointer(item);
        }
        item = fifos[currentIndex]->Peek();
        return GetPointer(item);

    }


    QueueDiscItem* Level_flex::pifopeek() {
	pifo.Print();
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
	return fifos[currentIndex]->GetNPackets();
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



    int Level_flex::getFifoTopValue() {

        int i = 0;

        int result = 0;

        while (true) {

            if (fifos[i]->Peek() != 0) {

                GearboxPktTag tag;

                GetPointer(fifos[i]->Peek())->GetPacket()->PeekPacketTag(tag);

                result = tag.GetDepartureRound();

                break;

            }

            // if there is no pkt in this level's fifos

            if (i == FIFO_PER_LEVEL - 1) {

                break;

            }

            i++;

        }

        return result;

    }

}
