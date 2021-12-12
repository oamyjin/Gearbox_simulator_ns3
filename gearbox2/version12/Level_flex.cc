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







	//cout<<endl;







	//cout<< "Level_flex fifoEnque<<<<<<<Start"<<endl;







        //level 0 or the pifo overflow to fifo







        NS_LOG_FUNCTION(this);







        







	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){

	        //cout << "Fifo overflow: fifo_n_pkt_size:"  << fifos[index]->GetNPackets() << " index:" << index << " DEFAULT_FIFO_N_SIZE:" << DEFAULT_FIFO_N_SIZE << endl;

		//for printing the departure round

		Packet* packet = GetPointer(item->GetPacket());

		GearboxPktTag tag;

		packet->PeekPacketTag(tag);

		//int departureRound = tag.GetDepartureRound();

		//cout << "The overflow pkt's departure round is "<<departureRound<<endl;



		//cout<< "Level_flex fifoEnque<<<<<<<End"<<endl;



		return item;

	}



        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

	//modify the fifoenque tag

	//fifoenque+1

	int fifoenque = tag.GetFifoenque() + 1;

	Replace_string h = Replace_string();

	h.FixNewFile(tag.GetUid(), "fifoenque",Simulator::Now().GetSeconds());

	tag.SetFifoenque(fifoenque);

	

	item->GetPacket()->ReplacePacketTag(tag);

        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));

        pkt_cnt++;



	

	//cout<<"The pkt is enqueued  to fifo successfully"<<endl;

	//cout<< "Level_flex fifoEnque<<<<<<<End"<<endl;

	return NULL;



    }







    // return the pkt being pushed out in this level

    QueueDiscItem* Level_flex::pifoEnque(QueueDiscItem* item) {

	//cout<<endl;

	//cout<< "Level_flex pifoEnque<<<<<<<Start"<<endl;

        NS_LOG_FUNCTION(this);

        // get the tag of item

        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

        int departureRound = tag.GetDepartureRound();

        // enque into pifo, if havePifo && ( < maxValue || < L)

	////cout << "PE= departureRound:" << departureRound << " getPifoMaxValue():" << getPifoMaxValue() << " pifo.Size():" << pifo.Size() << " pifo.LowestSize():" << pifo.LowestSize() << endl;

        if (departureRound < getPifoMaxValue() || isAllFifosEmpty()) {



            QueueDiscItem* re = pifo.Push(item, departureRound);

	    if((tag.GetUid() == 51)){

		cout<<tag.GetUid()<<" enque pifo "<<endl;

	    }

	    item->GetPacket()->PeekPacketTag(tag);

	    //cout<<"!!!!!!!!!!level pifoenque "<<tag.GetPifoenque()<<endl;

            if (re != 0) {

		

                GearboxPktTag tag1;

                re->GetPacket()->PeekPacketTag(tag1);
		//cout<<"The exceeded pkt is "<<tag1.GetUid()<<endl;

		//cout<<"Enque this pkt to this level's fifo"<<endl;

		int departureround = tag1.GetDepartureRound();

	    

	        re = fifoEnque(re, cal_index(departureround));// redirect to this level's fifo



		//if successfully enqueued to fifo, return 0; if overflow, return the pkt

            }

	    //cout<< "Level_flex pifoEnque<<<<<<<End"<<endl;

	    return re; // return the overflow pkt

        }

        // enque the enqued pkt into fifo     

        else {

	    int departureround = tag.GetDepartureRound();

	    

	    QueueDiscItem* re = fifoEnque(item, cal_index(departureround));

	    

            //cout<< "Level_flex pifoEnque<<<<<<<End"<<endl;

	    if((tag.GetUid() == 51)){

		cout<<tag.GetUid()<<" enque fifo "<<cal_index(departureround)<<endl;

	    }

            return re;

        }



    }







    QueueDiscItem* Level_flex::fifoDeque(int index) {//for reload



        if (isSelectedFifoEmpty(index)) {

            return 0;

        }

        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());

	

	//modify the fifodeque tag



	//fifodeque+1



	GearboxPktTag tag;

        GetPointer(item)->GetPacket()->PeekPacketTag(tag);



	





	int fifodeque = tag.GetFifodeque()+1;

	tag.SetFifodeque(fifodeque);

	Replace_string h = Replace_string();

	h.FixNewFile(tag.GetUid(), "fifodeque",Simulator::Now().GetSeconds());

	

        GetPointer(item)->GetPacket()->ReplacePacketTag(tag);

	//cout<<"fifodeque times is "<<fifodeque;

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

	////cout << "ifLowerThanLthenReload pifo.Size():" << pifo.Size() << " L:" << pifo.LowestSize() << " --------------" << endl;	

	//cout<<"reload_hold1"<<reload_hold<<endl;

        if(pifo.Size()<pifo.LowestSize() && reload_hold == 0){//!!!!must go through all the packets

		InitializeReload();



	}

	////cout << "reloaded --------------------------------------------" << endl;

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



		//cout<<"the earliest fifo"<<earliestFifo<<endl;

		if (earliestFifo != -1) {

			reload_hold = 1;					

			reload_fifoSize = getFifoNPackets(getEarliestFifo());

			reload_size = 0;

           	}		

	}







   // with speed up K





   int Level_flex::Reload(int k) {

	int npkts;

	    // if no pkt in fifos

		

            int earliestFifo = getEarliestFifo();

            if (earliestFifo == -1) {

		//cout << "earliestFifo:" << earliestFifo << endl;

                return 0;

            }



	    npkts = k < getFifoNPackets(earliestFifo) ? k : getFifoNPackets(earliestFifo);

	    remainingQ = k -npkts;

	    //cout<<"reload earliest fifo"<<earliestFifo<<"npkts"<<npkts<<"remainingQ"<<remainingQ<<endl;

	    for (int i = 0; i < npkts; i++){ 

		k--;       

		////cout << SPEEDUP_FACTOR - k << "th reload" << " pifoSize:" << pifo.Size() << endl;

		// load the reloaded pkt into pifo, the fifoDeque return pkt will not cause fifo overflow

		//pifoEnque(fifoDeque(earliestFifo));// don't change the current index

		QueueDiscItem* re = fifoDeque(earliestFifo);

		GearboxPktTag tag0;

		Packet* packet = GetPointer(re->GetPacket());

		packet->PeekPacketTag(tag0);

		if((tag0.GetUid() == 136)){

			cout<<tag0.GetUid()<<" reload from fifo "<<earliestFifo<<endl;

		}



		

		//get the reload times and set 

		int reload = tag0.GetReload()+1;

		tag0.SetReload(reload);

		Replace_string h = Replace_string();

		h.FixNewFile(tag0.GetUid(), "reload",Simulator::Now().GetSeconds());

		packet->ReplacePacketTag(tag0);

		//cout<<"reload times is "<<reload;

            	QueueDiscItem* re1 = pifo.Push(re, tag0.GetDepartureRound());	



		if(re1!=0){

			GearboxPktTag tag2;

			re1->GetPacket()->PeekPacketTag(tag2);



		}

	



		//GearboxPktTag tag;

		//re->GetPacket()->PeekPacketTag(tag);

	    	//cout<<"!!!!!!!!!!reload "<<tag.GetPifoenque()<<endl;

		



		if (re1 != NULL){

			GearboxPktTag tag;

			re1->GetPacket()->PeekPacketTag(tag);

			//cout << "redirect re:" << re1 << " dp:" << tag.GetDepartureRound() << endl;

			int departureround = tag.GetDepartureRound();	    

	    		fifoEnque(re1, cal_index(departureround));



		}

        }

	    return npkts;

    }





    int Level_flex::getEarliestFifo() {//won't find the current index







        for (int i = 1; i < DEFAULT_VOLUME; i++) {



            if (!fifos[(i + currentIndex) % DEFAULT_VOLUME]->IsEmpty()) {//start from currentindex+1 (if with migration) or from earliestfifo+1?



                return (i + currentIndex) % DEFAULT_VOLUME;



            }



        }







        return -1; // return -1 if all fifos are empty







    }







    const QueueItem* Level_flex::fifopeek1() {







	Ptr<const QueueItem> item;







	if (isCurrentFifoEmpty()) {







	    currentIndex = getEarliestFifo();







	    if (currentIndex == -1){







		currentIndex = 0;







		return NULL;







	    }







	    ////cout << " empty currentIndex:" << currentIndex << endl;







	}







	////cout << " currentIndex:" << currentIndex << endl;







	item = fifos[currentIndex]->Peek();







	return GetPointer(item);







    }



	const QueueItem* Level_flex::fifopeek2(int fifo) {







	Ptr<const QueueItem> item;







	if (isCurrentFifoEmpty()) {







	    currentIndex = getEarliestFifo();







	    if (currentIndex == -1){







		currentIndex = 0;







		return NULL;







	    }







	    ////cout << " empty currentIndex:" << currentIndex << endl;







	}







	////cout << " currentIndex:" << currentIndex << endl;







	item = fifos[fifo]->Peek();







	return GetPointer(item);







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







    bool Level_flex::isAllFifosEmpty(){//all fifos but not currenindex fifo







	for (int i=1;i<DEFAULT_VOLUME;i++){







		if(!isSelectedFifoEmpty((i + currentIndex) % DEFAULT_VOLUME)){







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



    int Level_flex::getRemainingQ() {

        return remainingQ;

    }







    void Level_flex::pifoPrint(){

	pifo.Print();

    }









    void Level_flex::fifoPrint(int index){

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



	if(reload_size>=reload_fifoSize){

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



	

	for (int i = 0; i < level; i++){

 

		term *= FIFO_PER_LEVEL;

		



	}



	return departureRound / term % FIFO_PER_LEVEL;



    }











}





