#include<stdio.h>
#include "ns3/Level_flex.h"

#include "ns3/Flow_pl.h"

#include <cmath>

#include <sstream>
#include <iostream>
#include <string> 

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
        pktCount = 0;
	flowNo = 0;
	this->isMigration = false;
        typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;
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
	    cout << i << " pifopeek:" << departureRound << endl;
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
		cout << "earliest fifopeek_depRound:" << departureRound << " pifopeek_tagmin:" << tag_min << endl;
		if (departureRound <= tag_min) { //deque level0 first?
		    //cout << "departureRound is "<< departureRound<<endl;
		    tag_min = departureRound;
		    level_min = 0;
		    cout << "level_min:" << level_min << " fifopeek_depRound:" << departureRound << " index:" << tag.GetIndex() << endl;
		    return 0;
		}
	}
	cout << "level_min:" << level_min << " tag_min:" << tag_min << endl;
	return level_min;
    }




    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {//to be considered
        this->currentRound = currentRound;
    }



    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {
        this->pktCount = pktCount;
    }


    int Gearbox_pl_fid_flex::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
        //departureRound = cal_theory_departure_round(currFlow, item->GetPacket()->GetSize());
	int curLastFinishRound = currFlow->getLastFinishRound();
        int departureRound = max(currentRound, curLastFinishRound);

	// update flow info
        float curWeight = currFlow->getWeight();
	cout << " curWeight:" << curWeight << endl;
        currFlow->setLastFinishRound(departureRound + int(curWeight));    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped

	return departureRound;
    }

    /*
    // Peixuan: This can be replaced by any other algorithms
    int Gearbox_pl_fid_flex::cal_theory_departure_round(Flow_pl* currFlow, int pkt_size) {
	//uint64_t flowlabel = uint64_t (ipHeader.GetSource ().Get ()) << 32 | uint64_t (ipHeader.GetDestination ().Get ());
        //string key = convertKeyValue(iph.GetFlowLabel());    // Peixuan 04212020 fid
        //Flow_pl* currFlow = this->getFlowPtr(flowlabel); // Peixuan 04212020 fid
        //float curWeight = currFlow->getWeight();
        int curLastFinishRound = currFlow->getLastFinishRound();
        int curStartRound = max(currentRound, curLastFinishRound);//curLastDepartureRound should be curLastFinishRound
        //int curFinishRound = curStarRound + curWeight;
        int curDepartureRound = (int)(curStartRound);
        return curDepartureRound;
    }*/



    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {
        NS_LOG_FUNCTION(this);
	cout << "L1:";
	levels[1].pifoPrint(); 
	cout << "L2:";
	levels[2].pifoPrint();   

	//if the packet is new (hasTag equals to 0), then calculate departure round
	//else (the packet is migrated), extract the departureRound
	GearboxPktTag tag;
	int departureRound = 0;
	bool hasTag = GetPointer(item)->GetPacket()->PeekPacketTag(tag);
   	cout<< " hasTag:"<<hasTag<<endl;
	
	bool result;

	// Migration pkt
	if(hasTag == 1){
	   int departureRound = tag.GetDepartureRound();
	   //level 2
	   if (departureRound - currentRound >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL)){
		// Update Pkt Tag
		cout << "L2" << endl;
		tag.SetIndex(departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
		GetPointer(item)->GetPacket()->ReplacePacketTag(tag);
		result = PifoEnqueue(2, GetPointer(item));
	    }
	    //level 1
	    else if (departureRound - currentRound >= FIFO_PER_LEVEL){
		cout << "L1" << endl;
		tag.SetIndex(departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
		GetPointer(item)->GetPacket()->ReplacePacketTag(tag);
		result = PifoEnqueue(1, GetPointer(item));
	    }
	    //level 0
	    else{
		cout << "L0" << endl;
		tag.SetIndex(departureRound % FIFO_PER_LEVEL);
		GetPointer(item)->GetPacket()->ReplacePacketTag(tag);
		result = FifoEnqueue(GetPointer(item), departureRound % FIFO_PER_LEVEL);
	    }
	}
	
	// NEW pkt: hasTag == 0
	else{
	   // get the enqueued pkt's flow
	   uint64_t flowlabel = getFlowLabel(item);
           Flow_pl* currFlow = getFlowPtr(flowlabel);
	   int insertLevel = currFlow->getInsertLevel();
	   cout << "insertLevel:" << insertLevel << endl;

	   // calculate rank value, which is the value of departure round in this case	
	   departureRound = RankComputation(GetPointer(item), currFlow);
	
           //if ((departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {
	   if (departureRound - currentRound >= FIFO_PER_LEVEL * FIFO_PER_LEVEL * FIFO_PER_LEVEL){
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
	
	   //level 2
	   if (departureRound - currentRound >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL) || insertLevel == 2){
		currFlow->setInsertLevel(2);
            	this->updateFlowPtr(flowlabel, currFlow);  // Peixuan 04212020 fid
		// Add Pkt Tag
	        GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL));
	    	cout << "GB L2 NEW ENQUE departureRound:" << departureRound  << " currentRound:" << currentRound << " index:" << departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL)%FIFO_PER_LEVEL << endl;
		result = PifoEnqueue(2, GetPointer(item));
	    }
	    //level 1
	    else if (departureRound - currentRound >= FIFO_PER_LEVEL || insertLevel == 1){
		currFlow->setInsertLevel(1);

       		this->updateFlowPtr(flowlabel, currFlow);  // Peixuan 04212020 fid
		// Add Pkt Tag

        	GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL));

	    	cout << "GB L1 NEW ENQUE departureRound:" << departureRound  << " currentRound:" << currentRound << " index:" << departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL << endl;
		result = PifoEnqueue(1, GetPointer(item));
	    }
	    //level 0
	    else{
		currFlow->setInsertLevel(0);

            	this->updateFlowPtr(flowlabel, currFlow);
		GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));
	
	        cout << "GB L2 NEW ENQUE departureRound:" << departureRound << " currentRound:" << currentRound << " index:" << departureRound % FIFO_PER_LEVEL << endl;
		result = FifoEnqueue(GetPointer(item), departureRound % FIFO_PER_LEVEL);
	    }
	
	    // record to file
	    Record("EnqueuedPktsList", departureRound);
	}
	
	if (result == true){
            setPktCount(pktCount + 1);
	    cout << "ENQUEUED" << endl;
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque: " << item);
            return true;
	}
	else{// fifo overflow drop pkt
	    return false;
	}
    }




    bool Gearbox_pl_fid_flex::PifoEnqueue(int level, QueueDiscItem* item){
 	QueueDiscItem* re = levels[level].pifoEnque(item); // re may be input item or the last pifo item
	cout << " re:" << re << endl;
	if (re != 0){
		cout << "DROP!!! pifo redirect to fifo overflow _ level:" << level << endl;
		Drop(re);
		return true; // or fase???
	}
	return true;
    }

    bool Gearbox_pl_fid_flex::FifoEnqueue(QueueDiscItem* item, int index){
	QueueDiscItem* re = levels[0].fifoEnque(item, index);
	// if overflow, go to higher levels
	//int level = 0;
	/*while(re != 0){
		// drop if no higher level to enque
		if (level >= DEFAULT_VOLUME - 1){
			cout << "DROP!!! level:" << level << endl;
			Drop(re);
			cout << "DROP!!! highest level's fifo overflow" << endl;
			return false; // or true???
		}
		level = level + 1;
		cout << " level:" << level << endl;
		GearboxPktTag tag;
		re->GetPacket()->PeekPacketTag(tag);
		tag.SetIndex(cal_index(level, tag.GetDepartureRound()));
		re = levels[level].pifoEnque(item);
		cout << " again re:" << re << endl;
	}*/
	//cout << "return true level:" << level << endl;
	if (re != 0){
		cout << "DROP!!! fifo overflow _ level:" << 0 << endl;
		Drop(re);
		return true; // or fase???
	}
	return true;
    }
	
    int Gearbox_pl_fid_flex::cal_index(int level, int departureRound){
	int term = 1;
	for (int i = 0; i < level; i++){
		term *= FIFO_PER_LEVEL;
	}
	cout << "CalculateIndex:" << departureRound / term % FIFO_PER_LEVEL << endl;
	return departureRound / term % FIFO_PER_LEVEL;
    }


    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {
	Ptr<QueueDiscItem> re = NULL;
        if (pktCount == 0) {
            return 0;
        }
	cout << endl << endl;
	int earliestLevel = findearliestpacket(DEFAULT_VOLUME);
	// no pkts in any fifo or pifo
	if (earliestLevel == -1){
		return re;	
	}

	cout << "GB earliestLevel:" << earliestLevel;
	if(earliestLevel == 0){
		cout << " FifoDequeue" << endl;
		re = FifoDequeue(0);
	}
	else{
		cout << " PifoDequeue" << endl;
		re = PifoDequeue(earliestLevel);
	}
	
	// get dequeued pkt's tag
	GearboxPktTag tag;
        re->GetPacket()->PeekPacketTag(tag);
        int departureRound = tag.GetDepartureRound();
	// record to file
	Record("DequeuedPktsList", departureRound);

	return re;
   }

   void Gearbox_pl_fid_flex::Record(string fname, int departureRound){
	string path = "UDPResult/pktsList/";
	path.append(fname);

	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	//fprintf(fp, "%f", Simulator::Now().GetSeconds());
	fprintf(fp, "%d", departureRound);
	fprintf(fp, "\t");
	fclose(fp);
	FILE *fp2;
	fp2 = fopen("UDPResult/pktsList/LevelsSize", "a+"); //open and write
	fprintf(fp2, "%f", Simulator::Now().GetSeconds());
	fprintf(fp2, " %s", "vt:");
	fprintf(fp2, "%d", currentRound);
	fprintf(fp2, "\t%s", "L0 fifos:");
	fprintf(fp2, "%d", levels[0].getFifoTotalNPackets());
	fprintf(fp2, "%s", "\t\t");
	fprintf(fp2, "%s", "L1 fifos:");
	fprintf(fp2, "%d", levels[1].getFifoTotalNPackets());
	/*fprintf(fp2, "%s", " fifo[0]:");
	fprintf(fp2, "%d", levels[1].getFifoNPackets(0));
	fprintf(fp2, "%s", " fifo[1]:");
	fprintf(fp2, "%d", levels[1].getFifoNPackets(1));
	fprintf(fp2, "%s", " fifo[2]:");
	fprintf(fp2, "%d", levels[1].getFifoNPackets(2));*/
	fprintf(fp2, "%s", " pifo:");
	fprintf(fp2, "%d", levels[1].getPifoSize());
	fprintf(fp2, "%s", "\t\t");
	fprintf(fp2, "%s", "L2 fifos:");
	fprintf(fp2, "%d", levels[2].getFifoTotalNPackets());
	/*fprintf(fp2, "%s", " fifo[0]:");
	fprintf(fp2, "%d", levels[2].getFifoNPackets(0));
	fprintf(fp2, "%s", " fifo[1]:");
	fprintf(fp2, "%d", levels[2].getFifoNPackets(1));
	fprintf(fp2, "%s", " fifo[2]:");
	fprintf(fp2, "%d", levels[2].getFifoNPackets(2));*/
	fprintf(fp2, "%s", " pifo:");
	fprintf(fp2, "%d", levels[2].getPifoSize());
	fprintf(fp2, "%s", "\t\t");
	fprintf(fp2, "%s", "GB total:");
	fprintf(fp2, "%d", levels[0].getFifoTotalNPackets() + levels[1].getFifoTotalNPackets() + levels[1].getPifoSize() + levels[2].getFifoTotalNPackets() + levels[2].getPifoSize());
	fprintf(fp2, "\t%s", fname.data());
	fprintf(fp2, "\n");
	fclose(fp2);

	FILE *fpl0;
	fpl0 = fopen("UDPResult/pktsList/Level0", "a+"); //open and write
	fprintf(fpl0, "%f", Simulator::Now().GetSeconds());
	fprintf(fpl0, "\t%d", levels[0].getFifoTotalNPackets());
	fprintf(fpl0, "\n");
	fclose(fpl0);

	FILE *fpl1;
	fpl1 = fopen("UDPResult/pktsList/Level1", "a+"); //open and write
	fprintf(fpl1, "%f", Simulator::Now().GetSeconds());
	fprintf(fpl1, "\t%d", levels[1].getFifoTotalNPackets());
	fprintf(fpl1, "\n");
	fclose(fpl1);

	FILE *fpl2;
	fpl2 = fopen("UDPResult/pktsList/Level2", "a+"); //open and write
	fprintf(fpl2, "%f", Simulator::Now().GetSeconds());
	fprintf(fpl2, "\t%d", levels[2].getFifoTotalNPackets());
	fprintf(fpl2, "\n");
	fclose(fpl2);
   }

   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::FifoDequeue(int earliestLevel) {
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
	// when fifodeque to migrate, no need to update currentRound
	this->setCurrentRound(tag.GetDepartureRound());
	Ptr<QueueDiscItem> p = fifoitem;
	setPktCount(pktCount - 1);
	this->Migration(currentRound);
	return p;
   }



   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::PifoDequeue(int earliestLevel) {
   	QueueDiscItem* pifoitem = levels[earliestLevel].pifoDeque();
	GearboxPktTag tag;
	pifoitem->GetPacket()->PeekPacketTag(tag);
	this->setCurrentRound(tag.GetDepartureRound());
	Ptr<QueueDiscItem> p = pifoitem;
	setPktCount(pktCount - 1);
	this->Migration(currentRound);
	return p;
   }




   void Gearbox_pl_fid_flex::Migration(int currentRound){

	//int i = 0;
	//level 1
	cout << "Migration " << currentRound << " <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	int level1_currentFifo = currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL;
	cout << "level1_currentFifo:" << level1_currentFifo;
	levels[1].setCurrentIndex(level1_currentFifo);
	cout << " isCurrentFifoEmpty:" << levels[1].isCurrentFifoEmpty();
	cout << " fSize:" << levels[1].getFifoNPackets(level1_currentFifo);
	cout << " " << levels[1].getCurrentFifoNPackets() << endl;		
	//while(!levels[1].isCurrentFifoEmpty() && (i < SPEEDUP_FACTOR)){
	int npkts = levels[1].getCurrentFifoNPackets() < SPEEDUP_FACTOR ? levels[1].getCurrentFifoNPackets() : SPEEDUP_FACTOR;
	for(int i = 0; i < npkts; i++){
		//fifo deque
		cout << "getFifoNPackets:" << levels[1].getFifoNPackets(level1_currentFifo) << endl;
		Ptr<QueueDiscItem> item = levels[1].fifoDeque(level1_currentFifo);
		cout << "after fifodeque getFifoNPackets:" << levels[1].getFifoNPackets(level1_currentFifo) << endl;
		GearboxPktTag tag;
		Packet* packet = GetPointer(GetPointer(item)->GetPacket());	
		//Get departure round					
		packet->PeekPacketTag(tag);
		int departureRound = tag.GetDepartureRound();
		cout << "i:" << i << " fifodeque:"<< departureRound << " from level 1 index " << tag.GetIndex() << endl;
		cout << "doenqueue" << endl;
		DoEnqueue(item);
		cout << "Migrated packet: "<< departureRound << endl;		
	}
	
	//int j = 0;
	int level2_currentFifo = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
	levels[2].setCurrentIndex(level2_currentFifo);	
	cout << " level2_currentFifo:" << level2_currentFifo << " isCurrentFifoEmpty():" << levels[2].isCurrentFifoEmpty() << " fifoSize:" << levels[2].getFifoNPackets(level2_currentFifo)  << " " << levels[2].getCurrentFifoNPackets() << endl;	
	npkts = levels[2].getCurrentFifoNPackets() < SPEEDUP_FACTOR ? levels[2].getCurrentFifoNPackets() : SPEEDUP_FACTOR;	
	//while(!levels[2].isCurrentFifoEmpty() && (j < SPEEDUP_FACTOR)){
	for(int i = 0; i < npkts; i++){
		//fifo deque
		cout << "fifodeque getFifoNPackets:" << levels[2].getFifoNPackets(level2_currentFifo) << endl;
		Ptr<QueueDiscItem> item = levels[2].fifoDeque(level2_currentFifo);
		cout << "after fifodeque getFifoNPackets:" << levels[2].getFifoNPackets(level2_currentFifo) << endl;

		GearboxPktTag tag;
		Packet* packet = GetPointer(GetPointer(item)->GetPacket());	
		//Get departure round					
		packet->PeekPacketTag(tag);
		int departureRound = tag.GetDepartureRound();
		cout << "i:" << i << " fifodeque:"<< departureRound << " from level 2 index " << tag.GetIndex() << endl;
		//delete tag  
		//packet->RemovePacketTag(tag);
		//enque to scheduler
		cout << "doenqueue" << endl;
		DoEnqueue(item);
		cout<<"Migrated packet: " << departureRound << " from level2 " << endl;
	}
	cout << "currentRound " << currentRound << " <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
   }



uint64_t Gearbox_pl_fid_flex::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	cout<<"source:"<<ipHeader.GetSource();
	cout<<" destination:"<<ipHeader.GetDestination();

	return uint64_t (ipHeader.GetSource ().Get ()) << 32 | uint64_t (ipHeader.GetDestination ().Get ());
}



 Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(uint64_t flowlabel) {
	
        string key = convertKeyValue(flowlabel);
	
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    int weight = DEFAULT_WEIGHT;
	    if (flowNo == 0){
		weight = 1;
	    }
	    else if (flowNo == 1){
		weight = 1;
	    }
	    else if (flowNo == 2){
		weight = 2;
	    }
	    flowNo++;
            cout << "a new flow comes" << endl;
	    return this->insertNewFlowPtr(flowlabel, weight, DEFAULT_BRUSTNESS);
        }

        return this->flowMap[key];
    }



    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(uint64_t fid, int weight, int brustness) { // Peixuan 04212020
        string key = convertKeyValue(fid);  // Peixuan 04212020
        //Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
        Flow_pl* newFlowPtr = new Flow_pl(fid, weight, brustness);
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
