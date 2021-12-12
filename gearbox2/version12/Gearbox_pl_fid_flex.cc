#include "ns3/Level_flex.h"



#include "ns3/Flow_pl.h"



#include "ns3/Replace_string.h"

#include <cmath>



#include <sstream>



#include <iostream>



#include <fstream>



#include <vector>



#include <string>



#include <map>



#include "ns3/ipv4-header.h"



#include "ns3/ppp-header.h"



#include "ns3/ipv6-header.h"

#include "ns3/tcp-header.h"



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



	flowNo = 0;



        typedef std::map<string, Flow_pl*> FlowMap;



        FlowMap flowMap;



	for (int i =0;i<DEFAULT_VOLUME;i++){

		levels[i].SetLevel(i);

	}



    }







    int Gearbox_pl_fid_flex::findearliestpacket(int volume) {

	



        int level_min = -1;



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



	    ////cout << i << " pifopeek:" << departureRound << endl;



            if (departureRound < tag_min) {



                tag_min = departureRound;



                level_min = i;



            }



        }



        const QueueItem* item = levels[0].fifopeek1();

	



	// if there is any pkt in level0



        if (item != NULL){



		Packet* packet = GetPointer(item->GetPacket());



		GearboxPktTag tag;



		packet->PeekPacketTag(tag);



		int departureRound = tag.GetDepartureRound();

		//if(currentRound >=100&& currentRound<=160){

			//cout << "earliest fifopeek_depRound:" << departureRound << " pifopeek_tagmin:" << tag_min << endl;

		//}



		if (departureRound <= tag_min) { //deque level0 first?



		    



		    tag_min = departureRound;





		    level_min = 0;



		    ////cout << "level_min:" << level_min << " fifopeek_depRound:" << departureRound << " index:" << tag.GetIndex() << endl;



		    return 0;



		}



	}



	////cout << "level_min:" << level_min << " tag_min:" << tag_min << endl;



	return level_min;



    }







    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {

	if(this->currentRound <= currentRound){

		this->currentRound = currentRound;

	}

	



	int level0_currentFifo = currentRound % FIFO_PER_LEVEL;



	levels[0].setCurrentIndex(level0_currentFifo);



	int level1_currentFifo = currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL;

	ifCurrentFifoChange(1,level1_currentFifo);

	levels[1].setCurrentIndex(level1_currentFifo);

	levels[1].setPreviousIndex(level1_currentFifo);

	

	

	int level2_currentFifo = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;

	ifCurrentFifoChange(2,level2_currentFifo);

	levels[2].setCurrentIndex(level2_currentFifo);

	levels[2].setPreviousIndex(level2_currentFifo);



    }





    void Gearbox_pl_fid_flex::ifCurrentFifoChange(int i, int currentFifo) {

	

	if((levels[i].getPreviousIndex() != currentFifo) & !levels[i].ifLowerthanL() ){//if currentFifo change and occupancy >= L,stop reload

		levels[i].setReloadHold(0);

		levels[i].setRemainingQ(SPEEDUP_FACTOR);

	}

	



    }





    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {



        this->pktCount = pktCount;



    }















    int Gearbox_pl_fid_flex::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){



	// calculate the rank(departureRound) value



        //departureRound = cal_theory_departure_round(currFlow, item->GetPacket()->GetSize());



	int curLastFinishRound = currFlow->getLastFinishRound();



        int departureRound = max(currentRound, curLastFinishRound);







	return departureRound;



    }







    uint64_t Gearbox_pl_fid_flex::getFlowLabel(Ptr<QueueDiscItem> item){



	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);



	const Ipv4Header ipHeader = ipItem->GetHeader();

	TcpHeader header;

    	GetPointer(item)->GetPacket()->PeekHeader(header);



	//cout<<"source:"<<ipHeader.GetSource();



	//cout<<" destination:"<<ipHeader.GetDestination();



	uint64_t flowLabel =  uint64_t(ipHeader.GetSource ().Get ()) << 32 | uint32_t (header.GetSourcePort())<< 16 | uint16_t (header.GetDestinationPort());



	



	return flowLabel;



    }











    Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(uint64_t flowlabel) {



	



        string key = convertKeyValue(flowlabel);



	



	// insert new flows with assigned weight



        if (flowMap.find(key) == flowMap.end()) {



	    int weight = DEFAULT_WEIGHT;



	    if (flowNo == 0){



		weight = 2;



	    }



	    else if (flowNo == 1){



		weight = 2;



	    }



	    else if (flowNo == 2){



		weight = 2;



	    }



	    flowNo++;



            //cout << "The pkt belongs to a new flow." << endl;



	    return this->insertNewFlowPtr(flowlabel, weight, DEFAULT_BRUSTNESS);



        }







        return this->flowMap[key];



    }















    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {

	



        NS_LOG_FUNCTION(this);



	GearboxPktTag tag;

	Packet* packet = GetPointer(GetPointer(item)->GetPacket());

	bool hasTag = packet->PeekPacketTag(tag);

	

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);



	const Ipv4Header ipHeader = ipItem->GetHeader();



	int departureRound;



	bool result;



	if(hasTag == 0){

		uid = uid +1;

		//cout<<"The pkt is new."<<endl;



		// get the enqueued pkt's flow



	        uint64_t flowlabel = getFlowLabel(item);



           	Flow_pl* currFlow = getFlowPtr(flowlabel);



		// calculate rank value, which is the value of departure round in this case	



	   	departureRound = RankComputation(GetPointer(item), currFlow);

		RecordFlow(flowlabel,departureRound);

		tagRecord(uid, departureRound,item);



		Replace_string h = Replace_string();

		h.FixNewFile(tag.GetUid(), "enquetime", Simulator::Now().GetSeconds());

		

	

			



        	if ((departureRound - currentRound ) >= FIFO_PER_LEVEL * FIFO_PER_LEVEL * FIFO_PER_LEVEL) {



	    	//cout << " DROP!!! TOO LARGE DEPARTURE_ROUND!  departureRound:"  << departureRound << " currentRound:" << currentRound << endl;



            		Drop(item);



			dropCount = dropCount+1;

	

			this->setDropCount(dropCount);



            		return false;   // 07072019 Peixuan: exceeds the maximum round



        	}



        	int curBrustness = currFlow->getBrustness();



        	if ((departureRound - currentRound) >= curBrustness) {



	    		//cout << " DROP!!! TOO BURST!  departureRound:"  << departureRound << " currentRound:" << currentRound << " curBrustness:" << curBrustness << endl;



            		Drop(item);



			dropCount = dropCount+1;



			this->setDropCount(dropCount);



            		return false;   // 07102019 Peixuan: exceeds the maximum brustness



        	}		

		this->updateFlowPtr(departureRound, flowlabel, currFlow); 

	}

	else{

		departureRound = tag.GetDepartureRound();

	}



	result = LevelEnqueue(GetPointer(item), uid, departureRound);



	if (result == true){



	    if(hasTag==0){

		Record("EnqueuedPktsList.txt", departureRound,item);

		

		setPktCount(pktCount + 1);



		cum_pktCount += 1;

		string path = "GBResult/pktsList/";



		path.append("uid");



		FILE *fp;



		//cout<<path<<endl;



		fp = fopen(path.data(), "a+"); //open and write



		fprintf(fp, "%d", tag.GetUid());



		fprintf(fp, "\t");



		fclose(fp);



	    }



           



	    //cout << "The pkt is enqueued successfully" << endl;



	    //cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;



            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque " << packet);



            return true;



	}



	else{

	// fifo overflow drop pkt



	    //cout << "The pkt fail to be enqueued" << endl;



	    //cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;



	    return false;



	}



    }





    bool Gearbox_pl_fid_flex::LevelEnqueue(QueueDiscItem* item, int uid, int departureRound){

	bool result;

	GearboxPktTag tag;

	Packet* packet = GetPointer(item->GetPacket());

	bool hasTag = packet->PeekPacketTag(tag);



	//if there is inversion, for example, enqueued pkt is smaller than vt, enqueue to current fifo in level 0

	if(departureRound < currentRound){

		result = FifoEnqueue(item, cal_index(0,currentRound));

		return result;

	}

	//level 2

	if ((departureRound - currentRound) >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL)){

		

		if(hasTag == 0){

			AddTag(2, departureRound, uid, item);

			if((tag.GetUid() == 136)){

			cout<<tag.GetUid()<<" L2 depart"<<tag.GetDepartureRound()<<" set index "<<departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL<<endl;

			}

		}

		result = PifoEnqueue(2, item);

		

		

	}

	//level 1



	else if ((departureRound - currentRound) >= FIFO_PER_LEVEL){

		

		if(hasTag == 0){

			AddTag(1, departureRound, uid, item);

			if((tag.GetUid() == 136)){

			cout<<tag.GetUid()<<" L1 depart"<<tag.GetDepartureRound()<<" set index "<<departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL<<endl;

			}

		}

		result = PifoEnqueue(1, item);

		

	}

	//level 0

	else{   

		if(hasTag == 0){

			AddTag(0, departureRound, uid, item);

		}

		result = FifoEnqueue(item, cal_index(0,departureRound));

		

	}



	return result;

	



    }



    void Gearbox_pl_fid_flex::AddTag(bool level, int departureRound, int uid, QueueDiscItem* item){

	GearboxPktTag tag;

	Packet* packet = GetPointer(item->GetPacket());

	packet->PeekPacketTag(tag);



	float time = Simulator::Now().GetSeconds();

	packet->AddPacketTag(GearboxPktTag(departureRound, time, uid));

    }







    bool Gearbox_pl_fid_flex::PifoEnqueue(int level, QueueDiscItem* item){



	//cout<<endl;



	//cout<<"Gearbox PifoEnque <<<<<<<<<<<<<<<<Start"<<endl;



 	QueueDiscItem* re = levels[level].pifoEnque(item);



	//when re != 0, the reason is the corresponding fifo is full



	// if overflow, go to higher levels



	if(re != 0){// return pkt from fifos



		// drop if no higher level to enque



		if (level == DEFAULT_VOLUME - 1){



			//cout << "DROP!!! no higher level to enque" << endl;



			Drop(re);



			dropCount = dropCount+1;



			setDropCount(dropCount);



			return false; // or true???



		}



		GearboxPktTag tag;



		Packet* re_packet = GetPointer(re->GetPacket());



		re_packet->PeekPacketTag(tag);

		

		//if the overflow times == 2, drop it; 



		//if the overflow times == 1, increase the overflow times and enque to higher level



		int overflow = tag.GetOverflow();



		//cout<<"overflow=="<<overflow<<endl;



		overflow = overflow+1; //



		//cout<<"overflow+1=="<<overflow<<endl;



		if(overflow == 2){



			Drop(re);



			dropCount = dropCount+1;



			setDropCount(dropCount);



			return false;



		}



		else{   



			tag.SetOverflow(overflow);

			Replace_string h = Replace_string();

			h.FixNewFile(tag.GetUid(), "overflow",Simulator::Now().GetSeconds());

			re_packet->ReplacePacketTag(tag);



			re = levels[level+1].fifoEnque(re, cal_index(level,tag.GetDepartureRound()));



		}	



	} 



	//cout<<"Gearbox PifoEnque <<<<<<<<<<<<<<<<End"<<endl;

	GearboxPktTag tag;

	Packet* packet = GetPointer(item->GetPacket());

	packet->PeekPacketTag(tag);



	return true;//means level 1's corresponding fifo is full



    }



    bool Gearbox_pl_fid_flex::FifoEnqueue(QueueDiscItem* item, int index){



	//cout<<endl;



	//cout<<"Gearbox FifoEnqueue <<<<<<<<<<<<<<<<Start"<<endl;



	Packet* packet = GetPointer(item->GetPacket());



	GearboxPktTag tag;



	packet->PeekPacketTag(tag);



	//cout<<" index:" << tag.GetIndex()<<endl;



	QueueDiscItem* re = levels[0].fifoEnque(item, index);



	if((tag.GetUid() == 136)){

		cout<<"currentRound"<<currentRound<<endl;

		cout<<tag.GetUid()<<" enque fifo "<<index<<" in level 0"<<endl;

	}

 



	// if overflow, go to higher levels



	int level = 0;



	if(re != 0){//?if(re != 0)



		// drop if no higher level to enque



		if (level >= DEFAULT_VOLUME - 1){



			Drop(re);



			dropCount = dropCount+1;



			setDropCount(dropCount);



			return false; // or true???



		}



		



		GearboxPktTag tag;



		Packet* re_packet = GetPointer(re->GetPacket());



		re_packet->PeekPacketTag(tag);	



		//if the overflow times == 2, drop it; 



		//if the overflow times == 1, increase the overflow times and enque to higher level



		int overflow = tag.GetOverflow();



		overflow += 1; 



		if(overflow == 2){



			Drop(re);



			dropCount = dropCount+1;



			setDropCount(dropCount);



			return false;



		}



		else{



			tag.SetOverflow(1);

			Replace_string h = Replace_string();

			h.FixNewFile(tag.GetUid(), "overflow",Simulator::Now().GetSeconds());

			re_packet->ReplacePacketTag(tag);

			level = level + 1;

			re = levels[level].pifoEnque(re);



		}	



		



	}



	return true;



    }



	



    int Gearbox_pl_fid_flex::cal_index(int level, int departureRound){



	int term = 1;

	

	for (int i = 0; i < level; i++){

 

		term *= FIFO_PER_LEVEL;

		



	}



	////cout << "CalculateIndex:" << departureRound / term % FIFO_PER_LEVEL << endl;

	



	return departureRound / term % FIFO_PER_LEVEL;



    }







    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {





	//cout<<endl;



	//cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;



	int earliestLevel = findearliestpacket(DEFAULT_VOLUME);



	if(earliestLevel == -1){



		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out | std::ios::app);



		thr <<  Simulator::Now().GetSeconds() << " drop " << getDropCount() << " enque " << enqueCount<< " deque "<< dequeCount << " overflow " << overflowCount << " reload "<< reloadCount <<"migration"<<migrationCount<< std::endl;

		



		for(int i=0; i<int(Flowlist.size()); i++){



			uint64_t flowLabel = Flowlist.at(i);



			Flow_pl* currFlow = getFlowPtr(flowLabel);



			thr <<  Simulator::Now().GetSeconds() << " Flow"<< i<<" completion time: "<< currFlow->getFinishTime() - currFlow->getStartTime() << std::endl;



		}



		thr <<  Simulator::Now().GetSeconds() <<" Pkt queing delay: "<< queuingCount/cum_pktCount << std::endl;



		return NULL;



	}



	Ptr<QueueDiscItem> re = NULL;



	if(earliestLevel == 0){



		



		re = FifoDequeue(0);



	}



	else{



	



		re = PifoDequeue(earliestLevel);



	}

	this->Reload();

	this->Migration(currentRound);

	

	

	



	//Set the finish time of flow



	uint64_t flowlabel = getFlowLabel(re);



        Flow_pl* currFlow = getFlowPtr(flowlabel);



	currFlow->setFinishTime(Simulator::Now().GetSeconds());



	







	GearboxPktTag tag;



	Packet* packet = GetPointer(GetPointer(re)->GetPacket());



	packet->PeekPacketTag(tag);



	tag.SetDequeTime(Simulator::Now().GetSeconds());

	Replace_string h = Replace_string();

	h.FixNewFile(tag.GetUid(), "dequetime", Simulator::Now().GetSeconds());



	packet->ReplacePacketTag(tag);



	int departureround = tag.GetDepartureRound();



	Record("DequeuedPktsList.txt", departureround, re);



	PacketRecord("DequeuedPktsList.txt", departureround, re);



	



	//cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;



	return re;



	



   }







   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::FifoDequeue(int earliestLevel) {



	//cout<<endl;



	//cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;



	QueueDiscItem* fifoitem;



	if(levels[earliestLevel].isCurrentFifoEmpty()){



		int earliestFifo = levels[earliestLevel].getEarliestFifo();

		fifoitem = levels[earliestLevel].fifoDeque(earliestFifo);

		GearboxPktTag tag;



		fifoitem->GetPacket()->PeekPacketTag(tag);

		if((tag.GetUid() == 136)){

			cout<<tag.GetUid()<<" deque from fifo "<< earliestFifo<<" in level 0"<<endl;

		}

	}



	else{



		int currentIndex = levels[earliestLevel].getCurrentIndex();



		fifoitem = levels[earliestLevel].fifoDeque(currentIndex);

		GearboxPktTag tag;



		fifoitem->GetPacket()->PeekPacketTag(tag);

		if((tag.GetUid() == 136)){

			cout<<tag.GetUid()<<" deque from fifo "<< currentIndex<<" in level 0"<<endl;

		}

	}

	



	GearboxPktTag tag;



	fifoitem->GetPacket()->PeekPacketTag(tag);



	this->setCurrentRound(tag.GetDepartureRound());



	Ptr<QueueDiscItem> p = fifoitem;



	setPktCount(pktCount - 1);



	//cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from fifo in level "<< 0 <<endl;

	



	//cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;



	return p;



   }



   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::PifoDequeue(int earliestLevel) {



	//cout<<endl;



	//cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;



   	QueueDiscItem* pifoitem = levels[earliestLevel].pifoDeque();

	GearboxPktTag tag;



	pifoitem->GetPacket()->PeekPacketTag(tag);

	if((tag.GetUid() == 136)){

		cout<<tag.GetUid()<<" deque from pifo in level "<<earliestLevel<<endl;

	}



	



	this->setCurrentRound(tag.GetDepartureRound());



	Ptr<QueueDiscItem> p = pifoitem;



	setPktCount(pktCount - 1);



	//cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from pifo in level "<<earliestLevel<<endl;



	

	

	



	//cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;



	return p;



   }

   void Gearbox_pl_fid_flex::Reload(){

	if(currentRound >=100&& currentRound<=166){

		cout<<"vt: "<<currentRound<<endl;

		for(int i = 0; i<3; i++){

			

			levels[i].pifoPrint();

			for (int fifo = 0;fifo<10;fifo++){

				levels[i].fifoPrint(fifo);

				

	

			}

			

			

		}

		

		

	}

	for(int j = 1; j<3; j++){

		if(currentRound >=100&& currentRound<=166){

			cout<<"reload_hold"<<levels[j].getReloadHold()<<endl;

			cout<<"reload_fifosize"<<levels[j].getReloadFifoSize()<<endl;

		}

		if(levels[j].getReloadHold() == 1){

			levels[j].updateReloadSize(levels[j].Reload(SPEEDUP_FACTOR));

			

			while ( !levels[j].isAllFifosEmpty()&&levels[j].getRemainingQ() != 0){//is all fifos but currentindex fifo emtpy

				if(!levels[j].ifLowerthanL()){

					break;

				}

				levels[j].InitializeReload();								

				levels[j].updateReloadSize(levels[j].Reload(levels[j].getRemainingQ()));



			}

			if(levels[j].finishCurrentFifoReload()&&!levels[j].ifLowerthanL()){

				levels[j].setReloadHold(0);

				levels[j].setRemainingQ(SPEEDUP_FACTOR);



			}

								



		}



	}

    

   }







   void Gearbox_pl_fid_flex::Migration(int currentRound){



	//cout<<endl;



	//cout<<"Gearbox Migration <<<<<<<<<<<<<<<<Start"<<endl;



	//cout<<"Current Round is "<< currentRound<<endl;



	//level 1



	//while(!levels[1].isCurrentFifoEmpty() && (i < SPEEDUP_FACTOR)){



	for(int j = 1; j<3; j++){



		int npkts = levels[j].getCurrentFifoNPackets() < SPEEDUP_FACTOR ? levels[j].getCurrentFifoNPackets() : SPEEDUP_FACTOR;



		for(int i = 0; i < npkts; i++){



			//cout<<endl;



	    		//cout << "It's the " << i <<" time"<<endl;



			//fifo deque



			Ptr<QueueDiscItem> item = levels[j].fifoDeque(levels[j].getCurrentIndex());

			GearboxPktTag tag;

			Packet* packet = GetPointer(GetPointer(item)->GetPacket());							

			packet->PeekPacketTag(tag);



			if((tag.GetUid() == 136)){

				cout<<tag.GetUid()<<" migrate from level "<<j<<" fifo "<<levels[j].getCurrentIndex()<<endl;

			}



			

			//Get departure round	

			//int departureRound = tag.GetDepartureRound();



			//enque to scheduler



			//cout << "Migrated packet: "<< departureRound <<" from level "<<j<< endl;	



			DoEnqueue(item);



			//get migration times and set



			int migration = tag.GetMigration()+1;



			tag.SetMigration(migration);

			Replace_string h = Replace_string();

			h.FixNewFile(tag.GetUid(), "migration",Simulator::Now().GetSeconds());



			packet->ReplacePacketTag(tag);



		



			//cout<<"Migration times is "<<migration;	



		}	







	}



	//cout<<"Gearbox Migration <<<<<<<<<<<<<<<<End"<<endl;



   }







    void Gearbox_pl_fid_flex::setDropCount(int count){



	this->dropCount = count;



    }







    int Gearbox_pl_fid_flex::getDropCount(){



	return dropCount;



    }











    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(uint64_t fid, int weight, int brustness) { // Peixuan 04212020



	Flowlist.push_back(fid);



        string key = convertKeyValue(fid);  // Peixuan 04212020



        Flow_pl* newFlowPtr = new Flow_pl(fid, weight, brustness);



	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());



        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));



        return this->flowMap[key];



    }







    int Gearbox_pl_fid_flex::updateFlowPtr(int departureRound, uint64_t fid, Flow_pl* flowPtr) { // Peixuan 04212020



        string key = convertKeyValue(fid);  // Peixuan 04212020



	// update flow info



        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped



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







	void Gearbox_pl_fid_flex::Record(string fname, int departureRound, Ptr<QueueDiscItem> item){

		GearboxPktTag tag;

        	item->GetPacket()->PeekPacketTag(tag);

		int uid = tag.GetUid();

		int dp = tag.GetDepartureRound();



		string path = "GBResult/pktsList/";



		path.append(fname);



		FILE *fp;



		//cout<<path<<endl;



		fp = fopen(path.data(), "a+"); //open and write



		//fprintf(fp, "%f", Simulator::Now().GetSeconds());



		fprintf(fp, "%d", dp);



		fprintf(fp, "\t");



		fclose(fp);



		FILE *fp2;



		fp2 = fopen("GBResult/pktsList/LevelsSize", "a+"); //open and write



		fprintf(fp2, "%f", Simulator::Now().GetSeconds());



		fprintf(fp2, " %s", "vt:");



		fprintf(fp2, "%d", currentRound);



		fprintf(fp2, "\t%s", "L0 fifos:");



		fprintf(fp2, "%d", levels[0].getFifoTotalNPackets());



		fprintf(fp2, "%s", "\t\t");



		fprintf(fp2, "%s", "L1 fifos:");



		fprintf(fp2, "%d", levels[1].getFifoTotalNPackets());



		fprintf(fp2, "%s", " pifo:");



		fprintf(fp2, "%d", levels[1].getPifoSize());



		fprintf(fp2, "%s", "\t\t");



		fprintf(fp2, "%s", "L2 fifos:");



		fprintf(fp2, "%d", levels[2].getFifoTotalNPackets());



		fprintf(fp2, "%s", " pifo:");



		fprintf(fp2, "%d", levels[2].getPifoSize());



		fprintf(fp2, "%s", "\t\t");



		fprintf(fp2, "%s", "GB total:");



		fprintf(fp2, "%d", levels[0].getFifoTotalNPackets() + levels[1].getFifoTotalNPackets() + levels[1].getPifoSize() + levels[2].getFifoTotalNPackets() + levels[2].getPifoSize());



		fprintf(fp2, "\t%s", fname.data());

		

		if((uid == 8375)| (uid == 8371) | (uid == 8508)){

			fprintf(fp2, "%d", uid);

		}



		fprintf(fp2, "\n");



		fclose(fp2);



		FILE *fpl0;



		



		fpl0 = fopen("GBResult/pktsList/Level0", "a+"); //open and write



		fprintf(fpl0, "%f", Simulator::Now().GetSeconds());



		fprintf(fpl0, "\t%d", levels[0].getFifoTotalNPackets());



		fprintf(fpl0, "\n");



		fclose(fpl0);



		FILE *fpl1;



		fpl1 = fopen("GBResult/pktsList/Level1", "a+"); //open and write



		fprintf(fpl1, "%f", Simulator::Now().GetSeconds());



		fprintf(fpl1, "\t%d", levels[1].getFifoTotalNPackets());



		fprintf(fpl1, "\n");



		fclose(fpl1);



		FILE *fpl2;



		fpl2 = fopen("GBResult/pktsList/Level2", "a+"); //open and write



		fprintf(fpl2, "%f", Simulator::Now().GetSeconds());



		fprintf(fpl2, "\t%d", levels[2].getFifoTotalNPackets());



		fprintf(fpl2, "\n");



		fclose(fpl2);



   }



    void Gearbox_pl_fid_flex::RecordFlow(uint64_t flowlabel, int departureRound){

		string path = "GBResult/pktsList/";



		path.append(std::to_string(flowlabel));



		FILE *fp;



		//cout<<path<<endl;



		fp = fopen(path.data(), "a+"); //open and write



		//fprintf(fp, "%f", Simulator::Now().GetSeconds());



		fprintf(fp, "%d", departureRound);



		fprintf(fp, "\t");



		fclose(fp);

	

	}













    void Gearbox_pl_fid_flex::Rate(int enqueCount, int dequeCount, int overflowCount, int reloadCount, int migrationCount){



		curTime = Simulator::Now();



		if((curTime.GetSeconds () - prevTime1.GetSeconds ())>1){



			std::ofstream thr1 ("GBResult/enqueRate.dat", std::ios::out | std::ios::app);



			std::ofstream thr2 ("GBResult/dequeRate.dat", std::ios::out | std::ios::app);



			std::ofstream thr3 ("GBResult/overflowRate.dat", std::ios::out | std::ios::app);



			std::ofstream thr4 ("GBResult/reloadRate.dat", std::ios::out | std::ios::app);	

			std::ofstream thr5 ("GBResult/migrationRate.dat", std::ios::out | std::ios::app);	



			enquerate = (enqueCount - enque_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 



			thr1 <<  curTime.GetSeconds () << " " << enquerate << std::endl;







			dequerate = (dequeCount - deque_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 



			thr2 <<  curTime.GetSeconds () << " " << dequerate << std::endl;







			overflowrate = (overflowCount - overflow_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 



			thr3 <<  curTime.GetSeconds () << " " << overflowrate << std::endl;







			reloadrate = (reloadCount - reload_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 		



			thr4 <<  curTime.GetSeconds () << " " << reloadrate << std::endl;



			migrationrate = (migrationCount - migration_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 		



			thr5 <<  curTime.GetSeconds () << " " << migrationrate << std::endl;



			prevTime1 = curTime;



			enque_previous = enqueCount;



			deque_previous = dequeCount;



			overflow_previous = overflowCount;



			reload_previous = reloadCount;

			migration_previous = migrationCount;







		}











    }







    void Gearbox_pl_fid_flex::PacketRecord(string fname, int departureRound, Ptr<QueueDiscItem> item){



		//get pkt information

		GearboxPktTag tag;

        	item->GetPacket()->PeekPacketTag(tag);



		/*int fifoenque = tag.GetFifoenque();

		int fifodeque = tag.GetFifodeque();

		int pifoenque = tag.GetPifoenque();

		int pifodeque = tag.GetPifodeque();

		int overflow = tag.GetOverflow();

		int reload = tag.GetReload();

		int dp = tag.GetDepartureRound();

		int migration = tag.GetMigration();

		int uid = tag.GetUid();

		double queuingDelay = tag.GetDequeTime() - tag.GetEnqueTime();

		enqueCount += fifoenque + pifoenque;

  		dequeCount += fifodeque + pifodeque;

		overflowCount += overflow;

		reloadCount +=  reload;

		queuingCount += queuingDelay;

		migrationCount += migration;

		Rate(enqueCount,dequeCount,overflowCount,reloadCount,migrationCount);*/

		

		







   }

    void Gearbox_pl_fid_flex::tagRecord(int uid, int departureRound, Ptr<QueueDiscItem> item){

		//GearboxPktTag tag;

        	//item->GetPacket()->PeekPacketTag(tag);

		//int dp = tag.GetDepartureRound();

		//int uid = tag.GetUid();

		FILE *fp2;

		fp2 = fopen("GBResult/pktsList/tagRecord.txt", "a+"); //open and write

		fprintf(fp2, "%f", Simulator::Now().GetSeconds());

		fprintf(fp2, " %s", "vt:");

		fprintf(fp2, "%d", currentRound);

		fprintf(fp2, " %s", "dp:");

		fprintf(fp2, "%d", departureRound);

		fprintf(fp2, " %s", "uid:");

		fprintf(fp2, "%d", uid);

		fprintf(fp2, " %s", "fifoenque:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "fifodeque:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "pifoenque:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "pifodeque:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "overflow:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "reload:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "migration:");

		fprintf(fp2, "%d", 0);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "queueing_delay:");

		fprintf(fp2, "%lf", 0.000000);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "enquetime:");

		fprintf(fp2, "%lf", 0.000000);

		fprintf(fp2, "%s", " ");

		fprintf(fp2, "%s", "dequetime:");

		fprintf(fp2, "%lf",  0.000000);

		fprintf(fp2, "\n");

		fclose(fp2);



    }



}







	

