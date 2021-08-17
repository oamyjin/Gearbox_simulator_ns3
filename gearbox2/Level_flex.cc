#include "ns3/object-factory.h"
#include "ns3/Level_flex.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/pfifo-fast-queue-disc.h"

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
        factory.Set("MaxPackets", UintegerValue(2));

        for (int i = 0; i < volume; i++) {
            fifos[i] = GetPointer(factory.Create<Queue>());
            uint32_t maxSize = 100;
            fifos[i]->SetMaxPackets(maxSize);
        }
    }

    Level_flex::~Level_flex() {
        NS_LOG_FUNCTION(this);
    }

    void Level_flex::enque(QueueDiscItem* item, int index) {
	/*GearboxPktTag tag;
	item->GetPacket()->PeekPacketTag(tag);
	int departureRound = tag.GetDepartureRound();
	cout << "====enque() departureRound:" << departureRound << endl;*/
        if (getPifoMaxValue() > getFifoTopValue()){
	    this->fifoEnque(item, index);
	}
        else{
	    this->pifoEnque(item);
        }
    }       

    void Level_flex::enque(QueueDiscItem* item, int index, bool isPifoEnque) {
        if (isPifoEnque == 1){
	    this->pifoEnque(item);
	}
        else{
	    this->fifoEnque(item, index);
        }
    }    

    void Level_flex::fifoEnque(QueueDiscItem* item, int index) {
        NS_LOG_FUNCTION(this);
        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));
        pkt_cnt++;
    }

    QueueDiscItem* Level_flex::deque() {
        if (isCurrentFifoEmpty()) {
            return 0;
        }
        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[currentIndex]->Dequeue());
        pkt_cnt--;
        return GetPointer(item);
    }


    //gearbox2
    void Level_flex::pifoEnque(QueueDiscItem* item) {
        NS_LOG_FUNCTION(this);
	// get the tag of item
	GearboxPktTag tag;
	item->GetPacket()->PeekPacketTag(tag);
	int departureRound = tag.GetDepartureRound();
	cout << "pifoEnque() departureRound:" << departureRound << endl;
	// if if exceeds H
	if ((pifo.Size() > H_value || pifo.Size() == H_value) && (departureRound > getPifoMaxValue() || departureRound == getPifoMaxValue())) {
	    fifoEnque(item, tag.GetIndex());
	}       
	else{
	    // enque into pifo, get those exceed H_value
	    cout << "============================" << endl;
	    PacketTagIterator::Item ititem = item->GetPacket()->GetPacketTagIterator().Next();
            NS_ASSERT (ititem.GetTypeId ().HasConstructor ());
	    cout << "-1---------------------------" << endl;
	    Callback<ObjectBase *> constructor = ititem.GetTypeId ().GetConstructor ();
            NS_ASSERT (!constructor.IsNull ());
	    cout << "-2---------------------------" << endl;
	    ObjectBase *instance = constructor ();
	    cout << "-3---------------------------" << endl;
	    GearboxPktTag *tag = dynamic_cast<GearboxPktTag *> (instance);
	    cout << "-4---------------------------" << tag->GetInstanceTypeId () << endl;
	    ititem.GetTag (*tag);
	    cout << "-5---------------------------" << endl;
            cout << tag->GetDepartureRound() << " " << tag->GetIndex() << endl;
	    cout << "============================" << endl;	
	    vector<QueueDiscItem*> re = pifo.Push(item, departureRound);
	    cout << "re.size():" << re.size() << endl;
	    while(re.size() != 0){
		QueueDiscItem* reItem = re.back();
		GearboxPktTag tag;
		reItem->GetPacket()->PeekPacketTag(tag);
	        fifoEnque(reItem, tag.GetIndex()); // get the last item
		re.pop_back(); // delete the last item
	    }
	}
	ifLowerThanLthenReload();     
    }

    void Level_flex::ifLowerThanLthenReload() {
        while (pifo.Size() < pifo.LowestSize()) {
            int earliestFifo = getEarliestFifo();
            if (earliestFifo == -1) {
		cout << "fifos are empty" << endl;
                return;
            }
            setCurrentIndex(earliestFifo);
            while (!isCurrentFifoEmpty()) {
                pifoEnque(deque());
            }
        }
        while (pifo.Size() > pifo.HighestSize()){
            QueueDiscItem* lastPacket = pifo.PopFromBottom();
            fifoEnque(lastPacket, currentIndex);
        }      
    }   

    int Level_flex::getEarliestFifo() {
        for (int i = 0; i < volume; i++) {
            if (!fifos[i]->IsEmpty()) {
                return i;
            }
        }
        return -1;
    }
   
    QueueDiscItem* Level_flex::pifoDeque() {
        NS_LOG_FUNCTION(this);
        Ptr<QueueDiscItem> item = pifo.Pop();
        ifLowerThanLthenReload();
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
        }
        else {
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
        return sizeof(fifos) / sizeof(fifos[0]);
    }

    int Level_flex::get_level_pkt_cnt() {
        // get pkt_cntqs
        return pkt_cnt;
    }

    int Level_flex::getPifoMaxValue(){
	return pifo.GetMaxValue();
    }

    int Level_flex::getFifoTopValue(){
	int i = 0;
	int result = 0;
	while(true){
	    if (fifos[i]->Peek() != 0){
		GearboxPktTag tag;
		GetPointer(fifos[i]->Peek())->GetPacket()->PeekPacketTag(tag);
		result = tag.GetDepartureRound();
		break;
	    }
	    // if there is no pkt in this level's fifos
	    if (i == FIFO_PER_LEVEL - 1){
		break;
	    }
	    i++;
	}
	return result;
    }	
}
