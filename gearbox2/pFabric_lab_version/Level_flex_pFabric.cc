#include "ns3/object-factory.h"

#include "ns3/Level_flex_pFabric.h"

#include "ns3/packet.h"

#include "ns3/ptr.h"

#include "ns3/log.h"

#include "ns3/pfifo-fast-queue-disc.h"

#include "ns3/queue.h"

#include "ns3/queue-disc.h"

#include "ns3/Replace_string.h"

#include "ns3/simulator.h"





namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("Level_flex_pFabric");

    NS_OBJECT_ENSURE_REGISTERED(Level_flex_pFabric);



    TypeId Level_flex_pFabric::GetTypeId(void)

    {

      static TypeId tid = TypeId("ns3::Level_flex_pFabric")

            .SetParent<Object>()

            .SetGroupName("TrafficControl")

            .AddConstructor<Level_flex_pFabric>()

    	    .AddAttribute ("H_value",

                   "H threshold of Pifo",

                   IntegerValue (40),

                   MakeIntegerAccessor (&Level_flex_pFabric::H_value),

                   MakeIntegerChecker<int> ())

    	    .AddAttribute ("L_value",

                   "L threshold of Pifo",

                   IntegerValue (20),

                   MakeIntegerAccessor (&Level_flex_pFabric::L_value),

                   MakeIntegerChecker<int> ())

            ;

        return tid;

    }



    Level_flex_pFabric::Level_flex_pFabric() : Level_flex_pFabric(DEFAULT_VOLUME, 0) {

        NS_LOG_FUNCTION(this);

    }



    Level_flex_pFabric::Level_flex_pFabric(int vol, int index) {

        NS_LOG_FUNCTION(this);

        this->volume = vol;

        this->currentIndex = index;

	cout << "H:" << H_value << " L:" << L_value << endl;

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





    Level_flex_pFabric::~Level_flex_pFabric() {

        NS_LOG_FUNCTION(this);

    }



    QueueDiscItem* Level_flex_pFabric::fifoEnque(QueueDiscItem* item, int index) {

        NS_LOG_FUNCTION(this);



	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){

		std::ofstream thr6 ("GBResult/pktsList/fifo_overflow.dat", std::ios::out | std::ios::app);

		thr6 << Simulator::Now().GetSeconds() << " " << index << " " << fifos[index]->GetNPackets() << endl;

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

	//tag.SetFifoenque(tag.GetFifoenque() + 1);

	//item->GetPacket()->ReplacePacketTag(tag);



        pkt_cnt++;



	return NULL;

    }







    // return the pkt being pushed out in this level

    QueueDiscItem* Level_flex_pFabric::pifoEnque(QueueDiscItem* item) {

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





    QueueDiscItem* Level_flex_pFabric::fifoDeque(int index) {//for reload

        if (isSelectedFifoEmpty(index)) {

            return 0;

        }



        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());

	fifodeque = fifodeque + 1;



	//modify the fifodeque tag

	//fifodeque+1

	GearboxPktTag tag;

        GetPointer(item)->GetPacket()->PeekPacketTag(tag);

	//tag.SetFifodeque(tag.GetFifodeque() + 1);

	//item->GetPacket()->ReplacePacketTag(tag);	



        pkt_cnt--;



        return GetPointer(item);

    }







    QueueDiscItem* Level_flex_pFabric::pifoDeque() {

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





    bool Level_flex_pFabric::ifLowerthanL(){

	if(pifo.Size()<pifo.LowestSize()){

		return true;

	}

	else{

		return false;

	}



    }



    void Level_flex_pFabric::InitializeReload() {



	int earliestFifo = getEarliestFifo();



	if (earliestFifo != -1) {

		reload_hold = 1;					

		reload_fifoSize = getFifoNPackets(getEarliestFifo());

		reload_size = 0;

		pifo.UpdateMaxValid(0); // pifo will not update maxtag automatically

		reloadIni = reloadIni + 1;

	}



    }



    void Level_flex_pFabric::TerminateReload(){

	reload_hold = 0;

	// the reload process is over, reset the flag and set the maxvalue to be the real (all pkts in the pifo) maxvalue

	pifo.UpdateMaxValid(1);

	pifo.UpdateMaxValue();	

    }













   // with speed up K

   int Level_flex_pFabric::Reload(int k) {

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

				QueueDiscItem* re2 = fifoEnque(re1, cal_index(departureround));

		    		if (re2 != NULL){

					removeSchePkt(tag.GetUid());

				}

				// reset the max tag value of pifo when the returned pkt's rank is smaller than the pifomaxvalue

				

				if (departureround < getPifoMaxValue()){

					setPifoMaxValue(departureround);

				}

			}

		}

		else{

			QueueDiscItem* re3 = fifoEnque(re, cal_index(tag0.GetDepartureRound()));

		    	if (re3 != NULL){  // will not overflow, because just deque this pkt from fifo

				removeSchePkt(tag0.GetUid());

			}

		}



        }

	return npkts;



    }





    int Level_flex_pFabric::getEarliestFifo() {//won't find the current index



        for (int i = 0; i < DEFAULT_VOLUME; i++) {

            if (!fifos[(i + currentIndex) % DEFAULT_VOLUME]->IsEmpty()) {//start from currentindex+1 (if with migration) or from earliestfifo+1?

                return (i + currentIndex) % DEFAULT_VOLUME;

            }

        }



        return -1; // return -1 if all fifos are empty

    }





    QueueDiscItem* Level_flex_pFabric::pifopeek() {

	return pifo.Peek();

    }





    void Level_flex_pFabric::setCurrentIndex(int index) {

        currentIndex = index;

    }





    void Level_flex_pFabric::setRemainingQ(int remaining) {

        remainingQ = remaining;

    }





    void Level_flex_pFabric::getAndIncrementIndex() {

        if (currentIndex + 1 < volume) {

            currentIndex++;

        }

        else {

     	     currentIndex = 0;

        }

    }





    bool Level_flex_pFabric::isCurrentFifoEmpty() {

        return fifos[currentIndex]->IsEmpty();

    }





    bool Level_flex_pFabric::isSelectedFifoEmpty(int index) {

        return fifos[index]->IsEmpty();

    }





    bool Level_flex_pFabric::isAllFifosEmpty(){

	for (int i=0;i<DEFAULT_VOLUME;i++){

		if(!isSelectedFifoEmpty(i)){

			return false;

		}

	}

	return true;

    }





    int Level_flex_pFabric::getCurrentIndex() {

        return currentIndex;

    }





    int Level_flex_pFabric::getCurrentFifoSize() {

        return fifos[currentIndex]->GetNBytes();

    }





    int Level_flex_pFabric::getCurrentFifoNPackets() {

	return getFifoNPackets(currentIndex);

    }





   int Level_flex_pFabric::getFifoMaxNPackets(){

	int max = fifos[0]->GetNPackets();

	for (int i = 1; i < FIFO_PER_LEVEL; i++){

		if ((unsigned)max < fifos[i]->GetNPackets()){

			max = fifos[i]->GetNPackets();

		}

	}

	return max;

   }





    int Level_flex_pFabric::getFifoNPackets(int index) {

	return fifos[index]->GetNPackets();

    }





    int Level_flex_pFabric::size() {

        return sizeof(fifos) / sizeof(fifos[0]);

    }





    int Level_flex_pFabric::getPifoMaxValue() {

        return pifo.GetMaxValue();

    }





    void Level_flex_pFabric::setPifoMaxValue(int max) {

        pifo.SetMaxValue(max);

    }





    int Level_flex_pFabric::getRemainingQ() {

        return remainingQ;

    }





    void Level_flex_pFabric::pifoPrint(){

	pifo.Print();

    }





    void Level_flex_pFabric::fifoPrint(int index){

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





    void Level_flex_pFabric::fifoPktPrint(int index){

	pifo.Print();



	for (int i=0;i<getFifoNPackets(index);i++)

  	{	

		Ptr<QueueItem> item = fifos[index]->Dequeue();

    		Packet* packet = GetPointer(item->GetPacket());

	 	GearboxPktTag tag;

		packet->PeekPacketTag(tag);

		if (tag.GetDepartureRound() == 3998 && tag.GetUid() == 3664){

			cout << "fifo " << index << ":" "(" << i << ": " << tag.GetDepartureRound() << ", " << tag.GetUid() << ", " << cal_index(tag.GetDepartureRound()) << ") " << endl;

		}

		fifos[index]->Enqueue(item);

  	}

    }





    int Level_flex_pFabric::getFifoTotalNPackets(){

	int cnt = 0; 

	for (int i = 0; i < DEFAULT_VOLUME; i++){  

		cnt += getFifoNPackets(i); 

	} 



	return cnt;    

    }





    int Level_flex_pFabric::getReloadFifoSize(){

	return reload_fifoSize;

    }





    int Level_flex_pFabric::getPifoSize(){

	return pifo.Size();

    }





    int Level_flex_pFabric::getReloadSize(){

	return reload_size;

    }





    void Level_flex_pFabric::updateReloadSize(int size) {

        reload_size = reload_size + size;

    }





    int Level_flex_pFabric::getReloadHold(){

	return reload_hold;

    }





    void Level_flex_pFabric::setReloadHold(int hold){

	reload_hold =hold;

    }





    void Level_flex_pFabric::setPreviousIndex(int idx){

	previous_idx =idx;

    }





    int Level_flex_pFabric::getPreviousIndex(){

	return previous_idx;

    }





    bool Level_flex_pFabric::finishCurrentFifoReload(){



	if(reload_size >= reload_fifoSize){

		return true;

	}	else{

		return false;

	}

    }





    void Level_flex_pFabric::SetLevel(int le){

	level = le;

    }





    int Level_flex_pFabric::cal_index(int departureRound){

	//cout << "departureRound:" << departureRound << " index:" << departureRound / GRANULARITY % FIFO_PER_LEVEL << endl; 

	return departureRound / GRANULARITY % FIFO_PER_LEVEL;

    }





    int Level_flex_pFabric::getPifoEnque(){

	return pifo.GetPifoEnque();

    }



    int Level_flex_pFabric::getPifoDeque(){

	return pifo.GetPifoDeque();

    }



    int Level_flex_pFabric::getFifoEnque(){

	return fifoenque;

    }



    int Level_flex_pFabric::getFifoDeque(){

	return fifodeque;

    }



    int Level_flex_pFabric::getReload(){

	return reload;

    }



    int Level_flex_pFabric::getReloadIni(){

	return reloadIni;

    }



    int Level_flex_pFabric::addSchePkt(int uid, int departureRound){

	qpkts[uid] = departureRound;

	return qpkts.size();

    }



    int Level_flex_pFabric::removeSchePkt(int uid){

	if (0 == qpkts.erase(uid)){

		cout << "cannot erase from map" << endl;

	}

	return qpkts.size();

    }





    int Level_flex_pFabric::cal_inversion_mag(int dp){

	int magnitude = 0;

	for (auto it = qpkts.begin(); it != qpkts.end(); ++it) {

		// compare the dequeued dp with all pkts' dp in the scheduler

		if (dp > it->second){

			magnitude += dp - it->second;

		}

	}

	return magnitude;

    }



}











