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
	
	maxUid = 0;



        typedef std::map<string, Flow_pl*> FlowMap;



        FlowMap flowMap;



	for (int i =0;i<DEFAULT_VOLUME;i++){

		levels[i].SetLevel(i);

	}



    }



    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {

	if(this->currentRound <= currentRound){

		this->currentRound = currentRound;

	}

	
	// there is only one level, with granularity of 100 = FIFO_PER_LEVEL * FIFO_PER_LEVEL 
	int level_currentFifo = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
	//ifCurrentFifoChange(level_currentFifo);
	levels[0].setCurrentIndex(level_currentFifo);

    }




/*
    void Gearbox_pl_fid_flex::ifCurrentFifoChange(int currentFifo) {

	

	if((levels[0].getPreviousIndex() != currentFifo) & !levels[0].ifLowerthanL() ){//if currentFifo change and occupancy >= L,stop reload
		cout << "levels[0].getPreviousIndex():" << levels[0].getPreviousIndex() << " currentFifo:" << currentFifo << endl;
		levels[0].setReloadHold(0);

		levels[0].setRemainingQ(SPEEDUP_FACTOR);

	}

	



    }

*/



    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {

        this->pktCount = pktCount;

    }




    int Gearbox_pl_fid_flex::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){

	// calculate the rank(departureRound) value

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

	//uint64_t flowLabel =  uint64_t(ipHeader.GetSource ().Get ()) << 32 | uint32_t (header.GetSourcePort()) << 16 | uint16_t (header.GetDestinationPort());
	uint64_t flowLabel =  uint64_t(ipHeader.GetSource ().Get ()) << 32 | uint16_t (header.GetSourcePort());

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
	    return this->insertNewFlowPtr(flowlabel, flowNo, weight, DEFAULT_BRUSTNESS);

        }

        return this->flowMap[key];

    }


    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {


        NS_LOG_FUNCTION(this);


	GearboxPktTag tag;
	

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);



	const Ipv4Header ipHeader = ipItem->GetHeader();



	int departureRound;



	uid = uid +1;
	
	maxUid = uid > maxUid ? uid : maxUid;



	uint64_t flowlabel = getFlowLabel(item);



        Flow_pl* currFlow = getFlowPtr(flowlabel);
	// calculate rank value, which is the value of departure round in this case	

	departureRound = RankComputation(GetPointer(item), currFlow);
	// coutMark
	if(currentRound >=3185&& currentRound<=3230){
		cout << " EN vt:" << currentRound << " dp:" << departureRound << endl;
		levels[0].pifoPrint();

		for (int fifo = 0;fifo<10;fifo++){

			levels[0].fifoPrint(fifo);

		}
	
	}
	

	//RecordFlow(flowlabel,departureRound);

	AddTag(0, departureRound, uid, GetPointer(item));

	//tagRecord(uid, departureRound,item);

	//Replace_string h = Replace_string();

	//h.FixNewFile(GetPointer(item), tag.GetUid(), "enquetime", Simulator::Now().GetSeconds());

		
	if ((departureRound - currentRound) >= FIFO_PER_LEVEL * FIFO_PER_LEVEL * FIFO_PER_LEVEL) {



	    	//cout << " DROP!!! TOO LARGE DEPARTURE_ROUND!  departureRound:"  << departureRound << " currentRound:" << currentRound << endl;



            	Drop(item);



		dropCount = dropCount+1;
		dropCountA = dropCountA+1;

	

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


	// enqueue into PIFO first
	// mark
	if(currentRound >3185&& currentRound<=3230){
		cout << "pifomax:" << levels[0].getPifoMaxValue() << " departureRound:" << departureRound << endl;
	}
	//cout << "====ENQ" << endl;
	QueueDiscItem* re = levels[0].pifoEnque(GetPointer(item));

	// redirect to fifo if not enque successfully into pifo 
	if (re != 0) {
		/*int index = departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
		if  (currentRound > departureRound){
			index = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
		}*/
		// re may no be the orginial item
		GearboxPktTag tag0;
		GetPointer(re->GetPacket())->PeekPacketTag(tag0);
		int index = tag0.GetDepartureRound() / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
		if  (currentRound > departureRound){
			index = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
		}
		//cout << "GB fifoenque dp:" << tag0.GetDepartureRound() << endl;
		re = levels[0].fifoEnque(re, index);

		// during reloading process, update pifomaxtag if the returned pkt's rank is smaller than the current pifomaxtag
		if (levels[0].getReloadHold() == 1 && tag0.GetDepartureRound() < levels[0].getPifoMaxValue()){
			levels[0].setPifoMaxValue(tag0.GetDepartureRound());
		}
		//Record("EnqueuedPktsList.txt", departureRound,item); //record enven dropped (drop the pkt that exceeds H)
		
		// coutMark
		if(currentRound >3185&& currentRound<=3230){
			cout << "Enqueue:" << departureRound << endl;
			levels[0].pifoPrint();
			for (int fifo = 0;fifo<10;fifo++){
				levels[0].fifoPrint(fifo);
			}
		}

		// fifo is full
		if (re != 0){
			Drop(re);
			dropCount = dropCount+1;
			dropCountB = dropCountB+1;
			this->setDropCount(dropCount);
			return false;
		}
		else {
			return true;
		}
	}
	else {
		// coutMark
		if(currentRound >=3185&& currentRound<=3230){
			cout << "Enqueue:" << departureRound << endl;
			levels[0].pifoPrint();
			for (int fifo = 0;fifo<10;fifo++){
				levels[0].fifoPrint(fifo);
			}
		}

		//Record("EnqueuedPktsList.txt", departureRound,item);

		setPktCount(pktCount + 1);

		cum_pktCount += 1;

		/*string path = "GBResult/pktsList/";

		path.append("uid");

		FILE *fp;

		fp = fopen(path.data(), "a+"); //open and write

		GearboxPktTag tag1;
		GetPointer(item->GetPacket())->PeekPacketTag(tag1);

		fprintf(fp, "%d", tag1.GetUid());

		fprintf(fp, "\t");

		fclose(fp);*/


		    //cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

		   //  NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque " << GetPionter(GetPointer(item))->GetPacket();
	     // coutMark
		if(currentRound >=3185&& currentRound<=3230){

			levels[0].pifoPrint();

			for (int fifo = 0;fifo<10;fifo++){

				levels[0].fifoPrint(fifo);

			}
	
		}
            return true;

	}


    }





    void Gearbox_pl_fid_flex::AddTag(bool level, int departureRound, int uid, QueueDiscItem* item){

	GearboxPktTag tag;

	Packet* packet = GetPointer(item->GetPacket());

	packet->PeekPacketTag(tag);



	float time = Simulator::Now().GetSeconds();

	packet->AddPacketTag(GearboxPktTag(departureRound, time, uid));

    }






    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {





	//cout<<endl;


	//cout << "====DQ" << endl;
	//cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;



	// no more pkts in GB
	if(levels[0].getPifoSize() == 0){
		/*currentRound = 0;
		FlowMap::iterator iter;
		iter = flowMap.begin();
		while(iter != flowMap.end()) {
			//cout << "EMPTY";
			Flow_pl* flowptr = iter->second;
			//cout << " dp:" << flowptr->getLastFinishRound() << "->";
			flowptr->setLastFinishRound(0);
			//cout << flowptr->getLastFinishRound() << endl;
			iter++;
		}*/



		//01152022
		int total = levels[0].getFifoEnque() + levels[0].getFifoDeque() + levels[0].getPifoEnque() + levels[0].getPifoDeque();
		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out | std::ios::app);
		thr <<  Simulator::Now().GetSeconds() << " TOTAL:" << total << " fifoenque:" << levels[0].getFifoEnque() << " fifodeque:" << levels[0].getFifoDeque()
			<< " pifoenque:" << levels[0].getPifoEnque() << " pifodeque:" << levels[0].getPifoDeque()
			<< " reload:"<< levels[0].getReload() << " reloadIni:" << levels[0].getReloadIni() 
			<< " migration:" << migrationCount << " overflow:" << overflowCount << " uid:" << maxUid << " vt:" << currentRound 
			<< " drop " << getDropCount() << " dropA " << dropCountA << " dropB " << dropCountB
			<< " flowNo:" << flowNo << " empty" << std::endl;


		



		/*for(int i=0; i<int(Flowlist.size()); i++){



			uint64_t flowLabel = Flowlist.at(i);



			Flow_pl* currFlow = getFlowPtr(flowLabel);



			thr <<  Simulator::Now().GetSeconds() << " Flow"<< i<<" completion time: "<< currFlow->getFinishTime() - currFlow->getStartTime() << std::endl;



		}*/



		//thr <<  Simulator::Now().GetSeconds() <<" Pkt queing delay: "<< queuingCount/cum_pktCount << std::endl;



		return NULL;



	}



	Ptr<QueueDiscItem> re = NULL;

	
	re = PifoDequeue();

	GearboxPktTag tag0;
	Packet* packet0 = GetPointer(GetPointer(re)->GetPacket());
	packet0->PeekPacketTag(tag0);
	//cout << "re:" << re << " dp:" << tag0.GetDepartureRound() << endl;
	// coutMark
	if(currentRound >=3185 && currentRound<=3230){
		cout << "DQ vt:" << currentRound << "  dp:" << tag0.GetDepartureRound() << " uid:" << tag0.GetUid() << endl;
	}
	this->Reload();

	//this->Migration(currentRound);


	//Set the finish time of flow



	uint64_t flowlabel = getFlowLabel(re);



        Flow_pl* currFlow = getFlowPtr(flowlabel);



	currFlow->setFinishTime(Simulator::Now().GetSeconds());


	GearboxPktTag tag;



	Packet* packet = GetPointer(GetPointer(re)->GetPacket());



	packet->PeekPacketTag(tag);



	tag.SetDequeTime(Simulator::Now().GetSeconds());

	//Replace_string h = Replace_string();

	//h.FixNewFile(GetPointer(re), tag.GetUid(), "dequetime", Simulator::Now().GetSeconds());



	packet->ReplacePacketTag(tag);



	int departureround = tag.GetDepartureRound();



	Record("DequeuedPktsList.txt", departureround, re);


	// statistic per 1000 packets dequeued
	if (departureround % 1000 == 0){
		int total = levels[0].getReload() + levels[0].getFifoEnque() + levels[0].getFifoDeque() + levels[0].getPifoEnque() + levels[0].getPifoDeque();
		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out | std::ios::app);
		thr <<  Simulator::Now().GetSeconds() << " drop " << getDropCount() << " TOTAL:" << total << " fifoenque:" << levels[0].getFifoEnque() << " fifodeque:" << levels[0].getFifoDeque()
				<< " pifoenque:" << levels[0].getPifoEnque() << " pifodeque:" << levels[0].getPifoDeque()
				<< " reload:"<< levels[0].getReload() << " reloadIni:" << levels[0].getReloadIni() 
				<< " migration:" << migrationCount << " overflow:" << overflowCount << " uid:" << maxUid << " vt:" << currentRound << " flowNo:" << flowNo << std::endl;
	}





	//PacketRecord("DequeuedPktsList.txt", departureround, re);
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

		
	}



	else{



		int currentIndex = levels[earliestLevel].getCurrentIndex();



		fifoitem = levels[earliestLevel].fifoDeque(currentIndex);

		GearboxPktTag tag;



		fifoitem->GetPacket()->PeekPacketTag(tag);

		
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



   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::PifoDequeue() {



	//cout<<endl;



	//cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;



   	QueueDiscItem* pifoitem = levels[0].pifoDeque();

	GearboxPktTag tag;



	pifoitem->GetPacket()->PeekPacketTag(tag);

	

	

	this->setCurrentRound(tag.GetDepartureRound());



	Ptr<QueueDiscItem> p = pifoitem;



	setPktCount(pktCount - 1);



	//cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from pifo in level "<<earliestLevel<<endl;

	//cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;


	return p;

   }


   void Gearbox_pl_fid_flex::Reload(){

	// coutMark
	if(currentRound >=3185&& currentRound<=3230){

		levels[0].pifoPrint();

		for (int fifo = 0;fifo<10;fifo++){

			levels[0].fifoPrint(fifo);

		}
	
	}

	int j = 0;
	//mark
	if(currentRound >=3185&& currentRound<=3230){
		cout << "levels[j].getReloadHold() :" << levels[j].getReloadHold() << endl;
	}
	// when reloadhold is equal to 1, reload packets
	if(levels[j].getReloadHold() == 1){
			
			levels[j].updateReloadSize(levels[j].Reload(SPEEDUP_FACTOR));

			while ( !levels[j].isAllFifosEmpty()&&levels[j].getRemainingQ() != 0){//is all fifos but currentindex fifo emtpy

				if(!levels[j].ifLowerthanL()){

					break;

				}
	
				levels[j].InitializeReload();	

				levels[j].updateReloadSize(levels[j].Reload(levels[j].getRemainingQ()));

			}
			//mark
			if(currentRound >=3185&& currentRound<=3230){
				cout << " RemainingQ:" << levels[j].getRemainingQ() << endl;
				cout << "levels[j].finishCurrentFifoReload():" << levels[j].finishCurrentFifoReload() << " levels[j].ifLowerthanL():" << levels[j].ifLowerthanL() << endl;
			}
		
			if(levels[j].finishCurrentFifoReload()){
				//cout << "TerminateReload" << endl;
				levels[j].TerminateReload();
				levels[j].setRemainingQ(SPEEDUP_FACTOR);

			}

			//mark
			if(currentRound >=3185&& currentRound<=3230){
				cout << "getReloadHold() :" << levels[j].getReloadHold() << " max:" << levels[j].getPifoMaxValue() << endl;
			}
		}

   }






/*
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

*/





    void Gearbox_pl_fid_flex::setDropCount(int count){



	this->dropCount = count;



    }







    int Gearbox_pl_fid_flex::getDropCount(){



	return dropCount;



    }











    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(uint64_t fid, int fno, int weight, int brustness) { // Peixuan 04212020



	Flowlist.push_back(fid);



        string key = convertKeyValue(fid);  // Peixuan 04212020

        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight, brustness);



	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());



        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));



        return this->flowMap[key];



    }







    int Gearbox_pl_fid_flex::updateFlowPtr(int departureRound, uint64_t fid, Flow_pl* flowPtr) { // Peixuan 04212020



        string key = convertKeyValue(fid);  // Peixuan 04212020



	// update flow info



        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped

	//cout << "departureRound:" << departureRound << "  flowPtr->getWeight():" <<  flowPtr->getWeight() << " departureRound + flowPtr->getWeight():" << departureRound + flowPtr->getWeight() << endl;

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

		//int uid = tag.GetUid();

		int dp = tag.GetDepartureRound();
		//int uid = tag.GetUid();


		string path = "GBResult/pktsList/";



		path.append(fname);



		FILE *fp;



		//cout<<path<<endl;



		fp = fopen(path.data(), "a+"); //open and write



		//fprintf(fp, "%f", Simulator::Now().GetSeconds());
		/*Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
		const Ipv4Header ipHeader = ipItem->GetHeader();
		TcpHeader header;
	    	GetPointer(item)->GetPacket()->PeekHeader(header);

		uint64_t flowlabel = getFlowLabel(item);
		Flow_pl* currFlow = getFlowPtr(flowlabel);

		fprintf(fp, "%s", "(");
		fprintf(fp, "%d", dp);
		fprintf(fp, "%s", ", ");
		fprintf(fp, "%d", currFlow->getFlowNo());
		fprintf(fp, "%s", ", ");
		fprintf(fp, "%d", uint32_t(ipHeader.GetSource().Get()));
		fprintf(fp, "%s", ", ");
		fprintf(fp, "%d", uint16_t (header.GetSourcePort()));
		fprintf(fp, "%s", ", ");
		fprintf(fp, "%f", Simulator::Now().GetSeconds());
		fprintf(fp, "%s", ")");*/



		fprintf(fp, "%d", dp);
		fprintf(fp, "\t");



		fclose(fp);

		



		/*FILE *fp2;



		fp2 = fopen("GBResult/pktsList/LevelsSize", "a+"); //open and write



		fprintf(fp2, "%f", Simulator::Now().GetSeconds());



		fprintf(fp2, " %s", "vt:");



		fprintf(fp2, "%d", currentRound);


		fprintf(fp2, "\t%s", "dp:");


		fprintf(fp2, "%d", departureRound);


		fprintf(fp2, "\t%s", "uid:");


		fprintf(fp2, "%d", uid);


		fprintf(fp2, "\t%s", "fifos:");



		fprintf(fp2, "%d", levels[0].getFifoTotalNPackets());



		fprintf(fp2, "%s", " pifo:");



		fprintf(fp2, "%d", levels[0].getPifoSize());


		fprintf(fp2, "%s", "\t\t");



		fprintf(fp2, "%s", "GB total:");



		fprintf(fp2, "%d", levels[0].getFifoTotalNPackets() + levels[0].getPifoSize());



		fprintf(fp2, "\t%s", fname.data());



		fprintf(fp2, "\n");



		fclose(fp2);*/



		FILE *fpl0;



		fpl0 = fopen("GBResult/pktsList/L0Fifo", "a+"); //open and write



		fprintf(fpl0, "%f", Simulator::Now().GetSeconds());



		fprintf(fpl0, "\t%d", levels[0].getFifoMaxNPackets());



		fprintf(fpl0, "\n");



		fclose(fpl0);

		
		FILE *fpl1;



		fpl1 = fopen("GBResult/pktsList/L0Pifo", "a+"); //open and write



		fprintf(fpl1, "%f", Simulator::Now().GetSeconds());



		fprintf(fpl1, "\t%d", levels[0].getPifoSize());



		fprintf(fpl1, "\n");



		fclose(fpl1);



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







	

