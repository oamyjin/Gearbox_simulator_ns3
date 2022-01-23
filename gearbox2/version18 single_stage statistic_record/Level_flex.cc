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



	//cout<< "Level_flex fifoEnque<<<<<<<Start"<<endl;

        NS_LOG_FUNCTION(this);

	if (!(fifos[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){

	        //cout << "Fifo overflow: fifo_n_pkt_size:"  << fifos[index]->GetNPackets() << " index:" << index << " DEFAULT_FIFO_N_SIZE:" << DEFAULT_FIFO_N_SIZE << endl;

		//for printing the departure round

		Packet* packet = GetPointer(item->GetPacket());

		GearboxPktTag tag;

		packet->PeekPacketTag(tag);

		return item;

	}



        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

	//modify the fifoenque tag

	//fifoenque+1

	//Replace_string h = Replace_string();
	//h.FixNewFile(item, tag.GetUid(), "fifoenque",Simulator::Now().GetSeconds());
	fifoenque = fifoenque + 1;
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
	//mark
	//cout << "departureRound:" << departureRound << " getPifoMaxValue():" << getPifoMaxValue() << " isAllFifosEmpty:" << isAllFifosEmpty() << endl;  
	if (departureRound < getPifoMaxValue() || isAllFifosEmpty()) {
	    //cout << "pifoPush" << endl; 
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
	//cout << "pifo.Size():" << pifo.Size() << " pifo.LowestSize():" << pifo.LowestSize() << " reload_hold:" << reload_hold << endl;

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

		//cout << "INITIALIZE RELOAD" << endl;

		//cout<<"the earliest fifo"<<earliestFifo<<endl;

		if (earliestFifo != -1) {

			reload_hold = 1;					

			reload_fifoSize = getFifoNPackets(getEarliestFifo());

			reload_size = 0;

			pifo.UpdateMaxValid(0); // pifo will not update maxtag automatically
			
			reloadIni = reloadIni + 1;
		}

	}

	void Level_flex::TerminateReload(){
		//cout << "TERMINATE RELOAD" << endl;
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

		//cout << "earliestFifo:" << earliestFifo << endl;

                return 0;

            }



	    npkts = k < getFifoNPackets(earliestFifo) ? k : getFifoNPackets(earliestFifo);

	    remainingQ = k - npkts;

	    // mark cout   
	    //cout<<"reload earliest fifo"<<earliestFifo<<"npkts"<<npkts<<"remainingQ"<<remainingQ<<endl;
	
	    // if start a new reloading process, set the pifomaxtag to be the max valid value of the range of the reloading fifo 
	    bool defaultMaxFlag = 0;
	    if (reload_size == 0){ 
		defaultMaxFlag = 1;
	    }

	    for (int i = 0; i < npkts; i++){ 

		k--;       
		reload = reload + 1;
		////cout << SPEEDUP_FACTOR - k << "th reload" << " pifoSize:" << pifo.Size() << endl;

		// load the reloaded pkt into pifo, the fifoDeque return pkt will not cause fifo overflow

		//pifoEnque(fifoDeque(earliestFifo));// don't change the current index

		QueueDiscItem* re = fifoDeque(earliestFifo);

		GearboxPktTag tag0;

		Packet* packet = GetPointer(re->GetPacket());

		packet->PeekPacketTag(tag0);

		//get and set the reload times 
		//cout << "--RELOAD dp:" << tag0.GetDepartureRound() << " reload:" << tag0.GetReload();
		//int reload = tag0.GetReload()+1;
		//cout << " +1:" << reload << endl;
		

		//tag0.SetReload(reload);

		//Replace_string h = Replace_string();

		//h.FixNewFile(re, tag0.GetUid(), "reload",Simulator::Now().GetSeconds());
		//reload = reload + 1;

		packet->ReplacePacketTag(tag0);
		
		if (defaultMaxFlag == 1){
			setPifoMaxValue((tag0.GetDepartureRound() / GRANULARITY) * GRANULARITY + GRANULARITY - 1);
			defaultMaxFlag = 0;
			//cout << " getPifoMaxValue:" << getPifoMaxValue() << endl;
		}
		
		//cout<<"reload times is "<<reload;

		//20220103 only when the pkt's tag <= pifomax, the pkt can be enqueued to pifo
		if (tag0.GetDepartureRound() <= getPifoMaxValue()){
            		QueueDiscItem* re1 = pifo.Push(re, tag0.GetDepartureRound());

			// mark cout
			/*if(tag0.GetDepartureRound() >7620&& tag0.GetDepartureRound()<=7620){
				cout << "reload_size:" << reload_size << " npkts:" << npkts << " k:" << k << endl;
				cout << "reload dp:" << tag0.GetDepartureRound() << " uid:" << tag0.GetUid() << endl;
				for (int ind = 0; ind < 10; ind++){
					fifoPrint(ind);
				}
				pifoPrint();
			}*/

			if (re1 != NULL){

				GearboxPktTag tag;

				re1->GetPacket()->PeekPacketTag(tag);

				int departureround = tag.GetDepartureRound();	    
				//cout << "pifo overflow fifoenque: dp:" << departureround << endl;
		    		fifoEnque(re1, cal_index(departureround));
			
				// reset the max tag value of pifo when the returned pkt's rank is smaller than the pifomaxvalue
				// mark
				/*if(departureround >7586 && departureround<=7620){
					cout << "------setPifoMaxValue:" << departureround << " getPifoMaxValue():" << getPifoMaxValue() << endl;
				}*/
				if (departureround < getPifoMaxValue()){
					setPifoMaxValue(departureround);
				}
			
				// mark cout
				//cout << "redirect dp:" << tag.GetDepartureRound() << " fifo:" << cal_index(departureround) << endl;

			}
		}

        }
		//cout << "RELOAD pifomaxtag:" << getPifoMaxValue() << endl;
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







    int Level_flex::get_level_pkt_cnt() {







        return pkt_cnt;







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


	//cout << "reload_size:" << reload_size << " reload_fifoSize:" << reload_fifoSize << endl;
	if(reload_size >= reload_fifoSize){ // TODO: original >=

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





