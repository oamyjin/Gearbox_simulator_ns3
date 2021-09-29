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









    /*

    void Level_flex::enque(QueueDiscItem* item, int index, bool isPifoEnque) {

        if (isPifoEnque == 1) {

	    cout << "pifo enque PifoSize:" << pifo.Size() << endl; 

            this->pifoEnque(item);

        }

        else {

	    cout << "fifo enque index:" << index << " NPkt:" << fifos[index]->GetNPackets() << endl; 

            this->fifoEnque(item, index);

        }

    }*/







    QueueDiscItem* Level_flex::fifoEnque(QueueDiscItem* item, int index) {

	cout<<endl;

	cout<< "Level_flex fifoEnque<<<<<<<Start"<<endl;

        //level 0 or the pifo overflow to fifo



        NS_LOG_FUNCTION(this);

        

	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){

	        cout << "Fifo overflow: fifo_n_pkt_size:"  << fifos[index]->GetNPackets() << " index:" << index << " DEFAULT_FIFO_N_SIZE:" << DEFAULT_FIFO_N_SIZE << endl;

		//for printing the departure round

		Packet* packet = GetPointer(item->GetPacket());

		GearboxPktTag tag;

		packet->PeekPacketTag(tag);

		int departureRound = tag.GetDepartureRound();

		cout << "The overflow pkt's departure round is "<<departureRound<<endl;

		//Drop(item);

		cout<< "Level_flex fifoEnque<<<<<<<End"<<endl;

		return item;

	}



        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));

        pkt_cnt++;

	cout<<"The pkt is enqueued  to fifo successfully"<<endl;

	cout<< "Level_flex fifoEnque<<<<<<<End"<<endl;

	return NULL;

    }







    // return the pkt being pushed out in this level

    QueueDiscItem* Level_flex::pifoEnque(QueueDiscItem* item) {

	cout<<endl;

	cout<< "Level_flex pifoEnque<<<<<<<Start"<<endl;

        NS_LOG_FUNCTION(this);



        // get the tag of item



        GearboxPktTag tag;



        item->GetPacket()->PeekPacketTag(tag);



        int departureRound = tag.GetDepartureRound();



        // enque into pifo, if havePifo && ( < maxValue || < L)

	//cout << "PE= departureRound:" << departureRound << " getPifoMaxValue():" << getPifoMaxValue() << " pifo.Size():" << pifo.Size() << " pifo.LowestSize():" << pifo.LowestSize() << endl;



        if (departureRound < getPifoMaxValue() || isAllFifosEmpty()) {

            QueueDiscItem* re = pifo.Push(item, departureRound);

            if (re != 0) {

		cout<<"The exceeded pkt is "<<re<<endl;

                GearboxPktTag tag1;

                re->GetPacket()->PeekPacketTag(tag1);

		cout<<"Enque this pkt to this level's fifo"<<endl;

                re = fifoEnque(re, tag1.GetIndex()); // redirect to this level's fifo

		//if successfully enqueued to fifo, return 0; if overflow, return the pkt

            }

	    cout<< "Level_flex pifoEnque<<<<<<<End"<<endl;

	    return re; // return the overflow pkt

        }



        // enque the enqued pkt into fifo     

        else {

	    QueueDiscItem* re = fifoEnque(item, tag.GetIndex());

            cout<< "Level_flex pifoEnque<<<<<<<End"<<endl;

            return re;

        }

    }







    QueueDiscItem* Level_flex::fifoDeque(int index) {//for reload

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

	setCurrentIndex(tag.GetIndex());

	//cout << "ifLowerThanLthenReload pifo.Size():" << pifo.Size() << " L:" << pifo.LowestSize() << " --------------" << endl;	

        ifLowerThanLthenReload();

	//cout << "reloaded --------------------------------------------" << endl;

        return GetPointer(item);

    }



   // with speed up K

   void Level_flex::ifLowerThanLthenReload() {

	int k = 0;

        while (pifo.Size() < pifo.LowestSize() && k < SPEEDUP_FACTOR) {

            int earliestFifo = getEarliestFifo();

            if (earliestFifo == -1) {

                break;

            }

            setCurrentIndex(earliestFifo);

	    int npkts = SPEEDUP_FACTOR < getFifoNPackets(earliestFifo) ? SPEEDUP_FACTOR : getFifoNPackets(earliestFifo);

	    for (int i = 0; i < npkts; i++){ 

		k++;       

		cout << k << "th reload" << " pifoSize:" << pifo.Size() << endl;

		pifoEnque(fifoDeque(earliestFifo));// don't change the current index

		if (pifo.Size() == pifo.LowestSize()){

		    break;

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



    bool Level_flex::isAllFifosEmpty(){

	for (int i=0;i<DEFAULT_VOLUME;i++){

		if(!isSelectedFifoEmpty(i)){

			return false;

		}

	}

	return true;



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



    void Level_flex::pifoPrint(){

	pifo.Print();

    }



    /*

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

    }*/

}
