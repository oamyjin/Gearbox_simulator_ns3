#include "ns3/Level_flex.h"



#include "ns3/Flow_pl.h"



#include <cmath>



#include <sstream>



#include <vector>



#include <string>







#include <map>







#include "ns3/ipv4-header.h"



#include "ns3/ppp-header.h"



#include "ns3/ipv6-header.h"



#include "ns3/ptr.h"



#include "Gearbox_pl_fid_flex.h"







#include "ns3/queue.h"



#include "ns3/queue-disc.h"



#include "ns3/log.h"



#include "ns3/simulator.h"

#include "ns3/ipv4-queue-disc-item.h"











using namespace std;







namespace ns3 {











    NS_LOG_COMPONENT_DEFINE("Gearbox_pl_fid_flex");



    NS_OBJECT_ENSURE_REGISTERED(Gearbox_pl_fid_flex);











    Gearbox_pl_fid_flex::Gearbox_pl_fid_flex() :Gearbox_pl_fid_flex(DEFAULT_VOLUME) {



    }







    Gearbox_pl_fid_flex::Gearbox_pl_fid_flex(int volume) {



        NS_LOG_FUNCTION(this);







        this->volume = volume;



        currentRound = 0;



        pktCount = 0; // 07072019 Peixuan



        typedef std::map<string, Flow_pl*> FlowMap;



        FlowMap flowMap;



    }











    int Gearbox_pl_fid_flex::findearliestpacket(int volume) {

        int level_min = 0;

        int tag_min = 99999999;



	// find the earliest pkt in pifos

        for (int i = 1; i < volume; i++) {

            QueueDiscItem* item = levels[i].pifopeek();

            if (item == NULL) {

                continue;

            }

            Packet* packet = GetPointer(item->GetPacket());

            GearboxPktTag tag;

            packet->PeekPacketTag(tag);

            int departureRound = tag.GetDepartureRound(); // tag of the peeked pkt

	    //cout << i << " pifopeek:" << departureRound << endl;

            if (departureRound < tag_min) {

                tag_min = departureRound;

                level_min = i;

            }

        }



        const QueueItem* item = levels[0].fifopeek();

	// if there is any pkt in level0

        if (item != NULL){

		Packet* packet = GetPointer(item->GetPacket());

		GearboxPktTag tag;

		packet->PeekPacketTag(tag);

		int departureRound = tag.GetDepartureRound();

		//cout << "earliest fifopeek_depRound:" << departureRound << " pifopeek_tagmin:" << tag_min << endl;

		if (departureRound <= tag_min) { //deque level0 first?

		    //cout << "departureRound is "<< departureRound<<endl;

		    tag_min = departureRound;

		    level_min = 0;

		    //cout << "level_min:" << level_min << " fifopeek_depRound:" << departureRound << " index:" << tag.GetIndex() << endl;

		    return 0;

		}

	}

	//cout << "level_min:" << level_min << " tag_min:" << tag_min << endl;

	return level_min;

    }









    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {//to be considered



        this->currentRound = currentRound;



    }







    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {



        this->pktCount = pktCount;



    }











    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {

        NS_LOG_FUNCTION(this);

	cout<<endl;

	cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

	cout << "CurrentRound:" << currentRound<< endl;

	cout <<"Before Enque the pifo state is: "<< endl;

	cout << "L1:";

	levels[1].pifoPrint(); 

	cout << "L2:";

	levels[2].pifoPrint();    	

   	

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);

	const Ipv4Header ipHeader = ipItem->GetHeader();

	cout<<"The address is: "<<endl;

	cout<<"source: "<<ipHeader.GetSource()<<" destination: "<<ipHeader.GetDestination()<<endl;

	uint64_t flowlabel = uint64_t (ipHeader.GetSource ().Get ()) << 32 | uint64_t (ipHeader.GetDestination ().Get ());

        string key = convertKeyValue(flowlabel); // Peixuan 04212020 fid





        if (flowMap.find(key) == flowMap.end()) {

            this->insertNewFlowPtr(flowlabel, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020 fid

            cout << "The pkt belongs to a new flow." << endl;

        }



        Flow_pl* currFlow = flowMap[key];       

	int insertLevel = currFlow->getInsertLevel();

	cout << "The insert level of this flow is: " << insertLevel<< endl;







	Packet* packet = GetPointer(GetPointer(item)->GetPacket());

        int pkt_size = packet->GetSize();

	GearboxPktTag tag;

	int departureRound;

	bool hasTag = packet->PeekPacketTag(tag);

	//if the packet is new, hasTag equals to 0, calculate departure round

	//else the packet is migrated, extract the departureRound



	if(hasTag == 0){

		cout<<"The pkt is new."<<endl;

           	departureRound = cal_theory_departure_round(ipHeader, pkt_size);

		departureRound = max(departureRound, currentRound);





        	if ((departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {

	    	cout << " DROP!!! TOO LARGE DEPARTURE_ROUND!  departureRound:"  << departureRound << " currentRound:" << currentRound << endl;

            	Drop(item);

            	return false;   // 07072019 Peixuan: exceeds the maximum round

        	}





        	int curBrustness = currFlow->getBrustness();

        	if ((departureRound - currentRound) >= curBrustness) {

	    	cout << " DROP!!! TOO BURST!  departureRound:"  << departureRound << " currentRound:" << currentRound << " curBrustness:" << curBrustness << endl;

            	Drop(item);

            	return false;   // 07102019 Peixuan: exceeds the maximum brustness

        	}





        	float curWeight = currFlow->getWeight();

        	currFlow->setLastFinishRound(departureRound + int(curWeight));    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped

        	this->updateFlowPtr(flowlabel, currFlow);  // Peixuan 04212020 fid

		

	}

	else{

		cout<< "The pkt should be migrated."<<endl;

	   	departureRound = tag.GetDepartureRound();

	}

        



	bool result;

	if(hasTag == 0){

		//level 2

		if (departureRound - currentRound >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL) || insertLevel == 2){

			currFlow->setInsertLevel(2);



            		this->updateFlowPtr(flowlabel, currFlow);  // Peixuan 04212020 fid

			// Add Pkt Tag



	        	packet->AddPacketTag(GearboxPktTag(departureRound, departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL));

			packet->PeekPacketTag(tag);

			cout<<"The storage destination is:"<<endl;			

			cout << "departureRound:" << departureRound << " insertLevel:" << 2 << " index:" << departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL<<endl;



			result = PifoEnqueue(2, GetPointer(item));

		}

		//level 1

		else if (departureRound - currentRound >= FIFO_PER_LEVEL || insertLevel == 1){

			currFlow->setInsertLevel(1);



            		this->updateFlowPtr(flowlabel, currFlow);  // Peixuan 04212020 fid

			// Add Pkt Tag



	        	packet->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL));

			packet->PeekPacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

	    		cout << "departureRound:" << departureRound << " insertLevel:" << 1 << " index:" << departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL<<endl;



			result = PifoEnqueue(1, GetPointer(item));

		}

		//level 0

		else{

			currFlow->setInsertLevel(0);



            		this->updateFlowPtr(flowlabel, currFlow);

			packet->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));

			packet->PeekPacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

	        	cout << "departureRound:" << departureRound << " insertLevel:" << 0 << " index:" << departureRound % FIFO_PER_LEVEL<<endl;



			result = FifoEnqueue(GetPointer(item), departureRound % FIFO_PER_LEVEL);

		}

	}

	else{

		//level 2

		if (departureRound - currentRound >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL)){

			// Update Pkt Tag



			tag.SetIndex(departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL);

			packet->ReplacePacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

			cout << "departureRound:" << departureRound << " insertLevel:" << 2 << " index:" << departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL<<endl;



			result = PifoEnqueue(2, GetPointer(item));

		}

		//level 1

		else if (departureRound - currentRound >= FIFO_PER_LEVEL){

			tag.SetIndex(departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);

			packet->ReplacePacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

			cout << "departureRound:" << departureRound << " insertLevel:" << 1 << " index:" << departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL<<endl;



			result = PifoEnqueue(1, GetPointer(item));

		}

		//level 0

		else{

			tag.SetIndex(departureRound % FIFO_PER_LEVEL);

			packet->ReplacePacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

			cout << "departureRound:" << departureRound << " insertLevel:" << 0 << " index:" << departureRound % FIFO_PER_LEVEL<<endl;



			result = FifoEnqueue(GetPointer(item), departureRound % FIFO_PER_LEVEL);

		}

	}



	if (result == true){

            setPktCount(pktCount + 1);

	    cout << "The pkt is enqueued successfully" << endl;

	    cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque " << packet << " Pkt:" << packet->GetUid());

            return true;

	}

	else{// fifo overflow drop pkt

	    cout << "The pkt fail to be enqueued" << endl;

	    cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	    return false;

	}

    }









    bool Gearbox_pl_fid_flex::PifoEnqueue(int level, QueueDiscItem* item){

	cout<<endl;

	cout<<"Gearbox PifoEnque <<<<<<<<<<<<<<<<Start"<<endl;



	Packet* packet = GetPointer(item->GetPacket());

	GearboxPktTag tag;

	packet->PeekPacketTag(tag);

	cout<<" index:" << tag.GetIndex()<<endl;



 	QueueDiscItem* re = levels[level].pifoEnque(item);

	//when re != 0, the reason is the corresponding fifo is full

	// if overflow, go to higher levels

	while(re != 0){//?if(re != 0)

		// drop if no higher level to enque

		if (level == DEFAULT_VOLUME - 1){

			cout << "DROP!!! no higher level to enque" << endl;

			Drop(re);

			return false; // or true???

		}

		level = level + 1;

		GearboxPktTag tag;

		Packet* re_packet = GetPointer(re->GetPacket());

		re_packet->PeekPacketTag(tag);

		tag.SetIndex(cal_index(level, tag.GetDepartureRound()));

		re_packet->ReplacePacketTag(tag);

		//if the overflow times == 2, drop it; 

		//if the overflow times == 1, increase the overflow times and enque to higher level

		int overflow = tag.GetOverflow();

		cout<<"overflow=="<<overflow<<endl;

		overflow = overflow+1; 

		cout<<"overflow+1=="<<overflow<<endl;

		if(overflow == 2){

			cout<<"Second time overflow"<< endl;

			cout<<"DROP!!! Second time overflow"<< endl;

			Drop(re);

			return false;

		}

		else{   

			cout<<"First time overflow"<< endl;

			cout<<"The pkt is enqueued to level "<< level <<endl;

			tag.SetOverflow(2);

			re_packet->ReplacePacketTag(tag);



			overflow = tag.GetOverflow();

			cout<<"overflow=="<<overflow<<endl;



			

			GearboxPktTag tag;

			re_packet->PeekPacketTag(tag);

			cout<<" overflow:" << tag.GetOverflow()<<endl;



			re = levels[level].pifoEnque(item);

		}	

	} 

	cout<<"Gearbox PifoEnque <<<<<<<<<<<<<<<<End"<<endl;

	return true;//means level 1's corresponding fifo is full

    }



    bool Gearbox_pl_fid_flex::FifoEnqueue(QueueDiscItem* item, int index){

	cout<<endl;

	cout<<"Gearbox FifoEnqueue <<<<<<<<<<<<<<<<Start"<<endl;



	Packet* packet = GetPointer(item->GetPacket());

	GearboxPktTag tag;

	packet->PeekPacketTag(tag);

	cout<<" index:" << tag.GetIndex()<<endl;



	QueueDiscItem* re = levels[0].fifoEnque(item, index);

	// if overflow, go to higher levels

	int level = 0;

	while(re != 0){//?if(re != 0)

		// drop if no higher level to enque

		if (level >= DEFAULT_VOLUME - 1){

			cout << "DROP!!! no higher level to enque" << endl;

			Drop(re);

			return false; // or true???

		}

		level = level + 1;

		GearboxPktTag tag;

		Packet* re_packet = GetPointer(re->GetPacket());

		re_packet->PeekPacketTag(tag);

		tag.SetIndex(cal_index(level, tag.GetDepartureRound()));

		re_packet->ReplacePacketTag(tag);

		//if the overflow times == 2, drop it; 

		//if the overflow times == 1, increase the overflow times and enque to higher level

		int overflow = tag.GetOverflow();

		overflow += 1; 

		cout<<"overflow+1=="<<overflow<<endl;

		if(overflow == 2){

			cout<<"Second time overflow"<< endl;

			cout<<"DROP!!! Second time overflow"<< endl;

			Drop(re);

			return false;

		}

		else{

			cout<<"First time overflow"<< endl;

			cout<<"The pkt is enqueued to level "<< level <<endl;

			tag.SetOverflow(2);

			re_packet->ReplacePacketTag(tag);



			overflow = tag.GetOverflow();

			cout<<"overflow=="<<overflow<<endl;



			Packet* packet = GetPointer(item->GetPacket());

			GearboxPktTag tag;

			packet->PeekPacketTag(tag);

			cout<<" overflow:" << tag.GetOverflow()<<endl;

			



			re = levels[level].pifoEnque(item);

		}	

		

	}

	return true;

    }

	

    int Gearbox_pl_fid_flex::cal_index(int level, int departureRound){

	int term = 1;

	for (int i = 0; i < level; i++){

		term *= FIFO_PER_LEVEL;

	}

	//cout << "CalculateIndex:" << departureRound / term % FIFO_PER_LEVEL << endl;

	return departureRound / term % FIFO_PER_LEVEL;

    }





    // Peixuan: This can be replaced by any other algorithms

    int Gearbox_pl_fid_flex::cal_theory_departure_round(Ipv4Header ipHeader, int pkt_size) {

	uint64_t flowlabel = uint64_t (ipHeader.GetSource ().Get ()) << 32 | uint64_t (ipHeader.GetDestination ().Get ());

        string key = convertKeyValue(flowlabel);

        //string key = convertKeyValue(iph.GetFlowLabel());    // Peixuan 04212020 fid

        Flow_pl* currFlow = this->getFlowPtr(flowlabel); // Peixuan 04212020 fid

        //float curWeight = currFlow->getWeight();

        int curLastFinishRound = currFlow->getLastFinishRound();

        int curStartRound = max(currentRound, curLastFinishRound);//curLastDepartureRound should be curLastFinishRound

        //int curFinishRound = curStarRound + curWeight;

        int curDepartureRound = (int)(curStartRound); // 07072019 Peixuan: basic test

        return curDepartureRound;

    }





    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {

	cout<<endl;

	cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

        if (pktCount == 0) {

		cout<<"The schedulor is empty"<<endl;

            	return 0;

        }

	int earliestLevel = findearliestpacket(DEFAULT_VOLUME);

	if(earliestLevel == 0){

		return FifoDequeue(0);

	}

	else{

		return PifoDequeue(earliestLevel);

	}

   }





   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::FifoDequeue(int earliestLevel) {

	cout<<endl;

	cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

	QueueDiscItem* fifoitem;

	if(levels[earliestLevel].isCurrentFifoEmpty()){

		int earliestFifo = levels[earliestLevel].getEarliestFifo();

		fifoitem = levels[earliestLevel].fifoDeque(earliestFifo);

	}

	else{

		int currentIndex = levels[earliestLevel].getCurrentIndex();

		fifoitem = levels[earliestLevel].fifoDeque(currentIndex);

	}



	GearboxPktTag tag;

	fifoitem->GetPacket()->PeekPacketTag(tag);

	this->setCurrentRound(tag.GetDepartureRound());

	Ptr<QueueDiscItem> p = fifoitem;

	setPktCount(pktCount - 1);



	cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from fifo in level "<< 0 <<endl;



	this->Migration(currentRound);

	cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	return p;

   }







   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::PifoDequeue(int earliestLevel) {

	cout<<endl;

	cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;



   	QueueDiscItem* pifoitem = levels[earliestLevel].pifoDeque();



	GearboxPktTag tag;



	pifoitem->GetPacket()->PeekPacketTag(tag);



	this->setCurrentRound(tag.GetDepartureRound());



	Ptr<QueueDiscItem> p = pifoitem;



	setPktCount(pktCount - 1);



	cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from pifo in level "<<earliestLevel<<endl;

	

	this->Migration(currentRound);

	cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	return p;



   }









   void Gearbox_pl_fid_flex::Migration(int currentRound){

	cout<<endl;

	cout<<"Gearbox Migration <<<<<<<<<<<<<<<<Start"<<endl;

	cout<<"Current Round is "<< currentRound<<endl;

	//level 1

	int level1_currentFifo = currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL;

	levels[1].setCurrentIndex(level1_currentFifo);

	cout<<endl;

	cout << "Level1_currentFifo:" << level1_currentFifo << " isCurrentFifoEmpty:" << levels[1].isCurrentFifoEmpty() << " fSize:" << levels[1].getFifoNPackets(level1_currentFifo) << " " << levels[1].getCurrentFifoNPackets() << endl;		

	//while(!levels[1].isCurrentFifoEmpty() && (i < SPEEDUP_FACTOR)){

	int npkts = levels[1].getCurrentFifoNPackets() < SPEEDUP_FACTOR ? levels[1].getCurrentFifoNPackets() : SPEEDUP_FACTOR;

	for(int i = 0; i < npkts; i++){

		cout<<endl;

	    	cout << "It's the " << i <<" time"<<endl;

		//fifo deque

		Ptr<QueueDiscItem> item = levels[1].fifoDeque(level1_currentFifo);

		GearboxPktTag tag;

		Packet* packet = GetPointer(GetPointer(item)->GetPacket());	

		//Get departure round					

		packet->PeekPacketTag(tag);

		int departureRound = tag.GetDepartureRound();

		//enque to scheduler

		cout << "Migrated packet: "<< departureRound <<" from level 1"<< endl;	

		DoEnqueue(item);



			

	}

	

	//int j = 0;

	int level2_currentFifo = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;

	levels[2].setCurrentIndex(level2_currentFifo);	

	cout<<endl;

	cout << "Level2_currentFifo:" << level2_currentFifo << " isCurrentFifoEmpty():" << levels[2].isCurrentFifoEmpty() << " fifoSize:" << levels[2].getFifoNPackets(level2_currentFifo)  << " " << levels[2].getCurrentFifoNPackets() << endl;	

	npkts = levels[2].getCurrentFifoNPackets() < SPEEDUP_FACTOR ? levels[2].getCurrentFifoNPackets() : SPEEDUP_FACTOR;	

	//while(!levels[2].isCurrentFifoEmpty() && (j < SPEEDUP_FACTOR)){

	for(int i = 0; i < npkts; i++){

		cout<<endl;

		cout << "It's the " << i <<" time"<<endl;

		//fifo deque

		Ptr<QueueDiscItem> item = levels[2].fifoDeque(level2_currentFifo);

		GearboxPktTag tag;

		Packet* packet = GetPointer(GetPointer(item)->GetPacket());	

		//Get departure round					

		packet->PeekPacketTag(tag);

		int departureRound = tag.GetDepartureRound();

		//enque to scheduler

		cout << "Migrated packet: "<< departureRound <<" from level 2"<< endl;	

		DoEnqueue(item);



	}

	cout<<"Gearbox Migration <<<<<<<<<<<<<<<<End"<<endl;

   }















 Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(uint64_t fid) {







        string key = convertKeyValue(fid);  // Peixuan 04212020



        Flow_pl* flow;



        if (flowMap.find(key) == flowMap.end()) {



            flow = this->insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020



        }



        flow = this->flowMap[key];



        return flow;



    }







    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(uint64_t fid, int weight, int brustness) { // Peixuan 04212020



        string key = convertKeyValue(fid);  // Peixuan 04212020



        Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);



        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));



        return this->flowMap[key];



    }







    int Gearbox_pl_fid_flex::updateFlowPtr(uint64_t fid, Flow_pl* flowPtr) { // Peixuan 04212020



        string key = convertKeyValue(fid);  // Peixuan 04212020



        this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));



        return 0;



    }







    string Gearbox_pl_fid_flex::convertKeyValue(uint64_t fid) { // Peixuan 04212020



        stringstream ss;



        ss << fid;  // Peixuan 04212020



        string key = ss.str();



        return key; //TODO:implement this logic



    }















    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoRemove(void) {



        //NS_LOG_FUNCTION (this);



        return 0;



    }







    Ptr<const QueueDiscItem> Gearbox_pl_fid_flex::DoPeek(void) const {



        //NS_LOG_FUNCTION (this);



        return 0;



    }







    Gearbox_pl_fid_flex::~Gearbox_pl_fid_flex()



    {



        NS_LOG_FUNCTION(this);



    }







    TypeId



        Gearbox_pl_fid_flex::GetTypeId(void)



    {



        static TypeId tid = TypeId("ns3::Gearbox_pl_fid_flex")



            .SetParent<QueueDisc>()



            .SetGroupName("TrafficControl")



            .AddConstructor<Gearbox_pl_fid_flex>()



            ;



        return tid;



    }











    bool Gearbox_pl_fid_flex::CheckConfig(void)



    {



        NS_LOG_FUNCTION(this);



        if (GetNQueueDiscClasses() > 0)



        {



            NS_LOG_ERROR("Gearbox_pl_fid_flex cannot have classes");



            return false;



        }







        if (GetNPacketFilters() != 0)



        {



            NS_LOG_ERROR("Gearbox_pl_fid_flex needs no packet filter");



            return false;



        }







        if (GetNInternalQueues() > 0)



        {



            NS_LOG_ERROR("Gearbox_pl_fid_flex cannot have internal queues");



            return false;



        }







        return true;



    }







    void Gearbox_pl_fid_flex::InitializeParams(void)



    {



        NS_LOG_FUNCTION(this);



    }







}
