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
	    cout << "i:" << i;
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





    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {
        NS_LOG_FUNCTION(this);

        Packet* packet = GetPointer(GetPointer(item)->GetPacket());

        //PppHeader* ppp = new PppHeader();

	//modify 

        //Header iph4;

        Ipv6Header iph;

        //cout<< "remove header"<<packet->RemoveHeader(&ppp)<<endl;

        packet->PeekHeader(iph);

        //cout<< "peek header"<<packet->PeekHeader(iph)<<endl;

        //cout<<"the source address is"<<iph4.GetSource()<<endl;

        //cout<<"the destination address is"<<iph4.GetDestination()<<endl;

        //Ipv6Header *ipv6Header = dynamic_cast<Ipv6Header*> (iph);

        int pkt_size = packet->GetSize();



        int departureRound = cal_theory_departure_round(iph, pkt_size);

        //cout<<"the calculated departureround is "<<departureRound<<endl;



        string key = convertKeyValue(iph.GetFlowLabel()); // Peixuan 04212020 fid

    //cout<<"the flow label is "<<key<<endl;

        // Not find the current key

        if (flowMap.find(key) == flowMap.end()) {

            this->insertNewFlowPtr(iph.GetFlowLabel(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020 fid

            cout << "a new flow comes" << endl;

        }

        Flow_pl* currFlow = flowMap[key];

        int insertLevel = currFlow->getInsertLevel();

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

        this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid

        // LEVEL_2
        if (departureRound - currentRound > (FIFO_PER_LEVEL * FIFO_PER_LEVEL) || departureRound - currentRound == (FIFO_PER_LEVEL * FIFO_PER_LEVEL) || insertLevel == 2) {
            currFlow->setInsertLevel(2);
            this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
	    // Add Pkt Tag
            GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL));
	    cout << "GB L2 NEW ENQUE departureRound:" << departureRound  << " currentRound:" << currentRound << " index:" << departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL << endl;
            levels[2].enque(GetPointer(item), departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL, 1);
           
        }
 	// LEVEL_1
        else if (departureRound - currentRound > FIFO_PER_LEVEL || departureRound - currentRound == FIFO_PER_LEVEL || insertLevel == 1) {
            this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
    	    // Add Pkt Tag
            GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL));
            currFlow->setInsertLevel(1);
	    cout << "GB L1 NEW ENQUE departureRound:" << departureRound << " currentRound:" << currentRound << endl;
            levels[1].enque(GetPointer(item), departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL, 1);
        }
	// LEVEL_0
        else {
            currFlow->setInsertLevel(0);
            this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
    	    // Add Pkt Tag
            GetPointer(item)->GetPacket()->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));
	    cout << "GB L2 NEW ENQUE departureRound:" << departureRound << " currentRound:" << currentRound << " index:" << departureRound % FIFO_PER_LEVEL << endl;
            levels[0].enque(GetPointer(item), departureRound % FIFO_PER_LEVEL, 0);
        }
        setPktCount(pktCount + 1);
        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque " << packet << " Pkt:" << packet->GetUid());
        return true;
    }



    // Peixuan: This can be replaced by any other algorithms

    int Gearbox_pl_fid_flex::cal_theory_departure_round(Ipv6Header iph, int pkt_size) {

        //int		fid_;	/* flow id */

        //int		prio_;

        // parameters in iph

        // TODO



        // Peixuan 06242019

        // For simplicity, we assume flow id = the index of array 'flows'



        string key = convertKeyValue(iph.GetFlowLabel());    // Peixuan 04212020 fid

        Flow_pl* currFlow = this->getFlowPtr(iph.GetFlowLabel()); // Peixuan 04212020 fid
        //float curWeight = currFlow->getWeight();
        int curLastFinishRound = currFlow->getLastFinishRound();

        int curStartRound = max(currentRound, curLastFinishRound);//curLastDepartureRound should be curLastFinishRound

        //int curFinishRound = curStarRound + curWeight;



        int curDepartureRound = (int)(curStartRound); // 07072019 Peixuan: basic test

        return curDepartureRound;

    }



    //06262019 Peixuan deque function of Gearbox:



    //06262019 Static getting all the departure packet in this virtual clock unit (JUST FOR SIMULATION PURPUSE!)




    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {
        if (pktCount == 0) {
            return 0;
        }
	cout << endl;
	int earliestLevel = findearliestpacket(DEFAULT_VOLUME);
	cout << "GB earliestLevel:" << earliestLevel;
	if(earliestLevel == 0){
		cout << " FifoDequeue" << endl;
		return FifoDequeue(0);
	}
	else{
		cout << " PifoDequeue" << endl;
		return PifoDequeue(earliestLevel);
	}
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

	int i = 0;
	//level 1
	cout << "Migration " << currentRound << " <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	int level1_currentFifo = currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL;
	levels[1].setCurrentIndex(level1_currentFifo);
	cout << " level1_currentFifo:" << level1_currentFifo << " isCurrentFifoEmpty():" << levels[1].isCurrentFifoEmpty() << endl;	
	while(!levels[1].isCurrentFifoEmpty() && (i < SPEEDUP_FACTOR)){
		//fifo deque
		Ptr<QueueDiscItem> item = levels[1].fifoDeque(level1_currentFifo);

		//Get departure round
		GearboxPktTag tag;
		Packet* packet = GetPointer(GetPointer(item)->GetPacket());		
		packet->PeekPacketTag(tag);		
		int departureRound = tag.GetDepartureRound();

		//enque
		// update tag value 
		tag.SetIndex(departureRound % FIFO_PER_LEVEL);
		GetPointer(GetPointer(item)->GetPacket())->ReplacePacketTag(tag);
		//from level 1 to level 0 
		levels[0].enque(GetPointer(item), departureRound % FIFO_PER_LEVEL, 0);//migrate stage by stage
		i++;
		cout << "Migrate packet: "<< departureRound << " to level 0 index: " << departureRound % FIFO_PER_LEVEL << endl;
	}

	
	int j = 0;
	int level2_currentFifo = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
	levels[2].setCurrentIndex(level2_currentFifo);	
	cout << " level2_currentFifo:" << level2_currentFifo << " isCurrentFifoEmpty():" << levels[2].isCurrentFifoEmpty() << endl;
	while(!levels[2].isCurrentFifoEmpty() && (j < SPEEDUP_FACTOR)){

		//fifo deque
		Ptr<QueueDiscItem> item = levels[2].fifoDeque(level2_currentFifo);

		//Get departure round
		GearboxPktTag tag;
		Packet* packet = GetPointer(GetPointer(item)->GetPacket());		
		packet->PeekPacketTag(tag);		
		int departureRound = tag.GetDepartureRound();

		//enque
		// update tag value 
		tag.SetIndex(departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
		GetPointer(GetPointer(item)->GetPacket())->ReplacePacketTag(tag);
		//from level 2 to level 1
		levels[1].enque(GetPointer(item), departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL, 1);
		j++;
		cout<<"Migrate packet: " << departureRound << " to level 1 index: "<< departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL << endl;
	}
	cout << "currentRound " << currentRound << " <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;

   }







 Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(int fid) {



        string key = convertKeyValue(fid);  // Peixuan 04212020

        Flow_pl* flow;

        if (flowMap.find(key) == flowMap.end()) {

            flow = this->insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020

        }

        flow = this->flowMap[key];

        return flow;

    }



    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020

        string key = convertKeyValue(fid);  // Peixuan 04212020

        Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);

        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));

        return this->flowMap[key];

    }



    int Gearbox_pl_fid_flex::updateFlowPtr(int fid, Flow_pl* flowPtr) { // Peixuan 04212020

        string key = convertKeyValue(fid);  // Peixuan 04212020

        this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));

        return 0;

    }



    string Gearbox_pl_fid_flex::convertKeyValue(int fid) { // Peixuan 04212020

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
