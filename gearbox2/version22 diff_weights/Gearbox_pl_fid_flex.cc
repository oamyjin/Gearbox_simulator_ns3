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
        pktCount = 0;
	flowNo = 0;
	maxUid = 0;
        typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;

	for (int i =0;i<DEFAULT_VOLUME;i++){
		levels[i].SetLevel(i);
	}

	ifstream infile;
	infile.open("/home/pc/ns-allinone-3.26/ns-3.26/scratch/weights.txt");
	int no = 0;
	infile >> no;
        for (int i = 0; i < no; i++){
		int temp = 0;
		infile >> temp;
		weights.push_back(temp);
	}
	infile.close();

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


    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {
        this->pktCount = pktCount;
    }


    int Gearbox_pl_fid_flex::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	int curLastFinishRound = currFlow->getLastFinishRound();
        int departureRound = max(currentRound, curLastFinishRound);
	return departureRound;
    }


    string Gearbox_pl_fid_flex::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	TcpHeader header;
    	GetPointer(item)->GetPacket()->PeekHeader(header);

	stringstream ss;
        ss << ipHeader.GetSource ().Get ()<<header.GetSourcePort()<<header.GetDestinationPort();  // Peixuan 04212020
        string flowLabel = ss.str();

	return flowLabel;
    }



    Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(string flowlabel) {
        string key = convertKeyValue(flowlabel);
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    //int weight = DEFAULT_WEIGHT;
	    int weight = weights.at(flowNo);
	    flowNo++;
	    return this->insertNewFlowPtr(flowlabel, flowNo, weight, DEFAULT_BURSTNESS);

        }
        return this->flowMap[key];
    }


    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {
        NS_LOG_FUNCTION(this);

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	int departureRound;

	uid = uid + 1;
	maxUid = uid > maxUid ? uid : maxUid;

	string flowlabel = getFlowLabel(item);
        Flow_pl* currFlow = getFlowPtr(flowlabel);

	// calculate rank value, which is the value of departure round in this case	
	departureRound = RankComputation(GetPointer(item), currFlow);

	// record the difference of the rank and vt to file
	rankDifference[departureRound - currentRound] += 1;

	std::ofstream thr ("GBResult/pktsList/rankVtDifference.dat", std::ios::out | std::ios::app);
	thr << departureRound - currentRound << " ";

	// Gearbox size and drop
	int total = levels[0].getPifoSize();
	for (int l = 0; l < FIFO_PER_LEVEL; l++){
		total += levels[0].getFifoNPackets(l);
	}
	std::ofstream thr2 ("GBResult/pktsList/GB_size.dat", std::ios::out | std::ios::app);
	thr2 << Simulator::Now().GetSeconds() << " " << total << endl;

	AddTag(0, departureRound, uid, GetPointer(item));
	//Record("EnqueuedPktsList.txt", departureRound, item);
		
	if ((departureRound - currentRound) >= FIFO_PER_LEVEL * FIFO_PER_LEVEL * FIFO_PER_LEVEL) {
            	Drop(item);

		std::ofstream thr3 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
		thr3 << Simulator::Now().GetSeconds() << " "  << 0 << endl;

		dropCount = dropCount+1;
		dropCountA = dropCountA+1;
		this->setDropCount(dropCount);

            	return false;   //exceeds the maximum round
        }

        int curBurstness = currFlow->getBurstness();

        if ((departureRound - currentRound) >= curBurstness) {
            	Drop(item);

		std::ofstream thr4 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
		thr4 << Simulator::Now().GetSeconds() << " "  << 0 << endl;

		dropCount = dropCount+1;
		this->setDropCount(dropCount);

            	return false;   // exceeds the maximum burstness

        }		
	this->updateFlowPtr(departureRound, flowlabel, currFlow); 

	// enqueue into PIFO first
	QueueDiscItem* re = levels[0].pifoEnque(GetPointer(item));

	// redirect to fifo if not enque successfully into pifo 
	if (re != 0) {
		// re may no be the orginial item
		GearboxPktTag tag0;
		GetPointer(re->GetPacket())->PeekPacketTag(tag0);
		int index = tag0.GetDepartureRound() / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
		if  (currentRound > departureRound){
			index = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
		}
		re = levels[0].fifoEnque(re, index);

		// during reloading process, update pifomaxtag if the returned pkt's rank is smaller than the current pifomaxtag
		if (levels[0].getReloadHold() == 1 && tag0.GetDepartureRound() < levels[0].getPifoMaxValue()){
			levels[0].setPifoMaxValue(tag0.GetDepartureRound());
		}

		// fifo is full
		if (re != 0){
			Drop(re);

			GearboxPktTag tag1;
			GetPointer(re->GetPacket())->PeekPacketTag(tag1);
			/*if (tag1.GetFifodeque() + tag1.GetPifodeque() != 0){
				std::ofstream thr5 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
				thr5 << Simulator::Now().GetSeconds() << " " << tag1.GetFifodeque() << " " << tag1.GetPifodeque() << endl;
				dropCountC += 1;
			}*/

			std::ofstream thr6 ("GBResult/pktsList/fifo_drop.dat", std::ios::out | std::ios::app);
			thr6 << (index - levels[0].getEarliestFifo() + FIFO_PER_LEVEL) % FIFO_PER_LEVEL << " ";

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
		setPktCount(pktCount + 1);
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
	// no more pkts in GB
	if(levels[0].getPifoSize() == 0){
		//01152022
		int total = levels[0].getFifoEnque() + levels[0].getFifoDeque() + levels[0].getPifoEnque() + levels[0].getPifoDeque();
		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out);
		thr <<  Simulator::Now().GetSeconds() << " TOTAL:" << total << " fifoenque:" << levels[0].getFifoEnque() << " fifodeque:" << levels[0].getFifoDeque()
			<< " pifoenque:" << levels[0].getPifoEnque() << " pifodeque:" << levels[0].getPifoDeque()
			<< " reload:"<< levels[0].getReload() << " reloadIni:" << levels[0].getReloadIni() 
			<< " uid:" << maxUid << " vt:" << currentRound 
                        << " drop:" << getDropCount() << " = dropA:" << dropCountA << " + dropB:" << dropCountB << " - dropC:" << dropCountC
			<< " flowNo:" << flowNo << " empty" << std::endl;

		return NULL;
	}

	Ptr<QueueDiscItem> re = NULL;
	
	re = PifoDequeue();

	GearboxPktTag tag0;
	Packet* packet0 = GetPointer(GetPointer(re)->GetPacket());
	packet0->PeekPacketTag(tag0);
	this->Reload();

	//Set the finish time of flow
	string flowlabel = getFlowLabel(re);
        Flow_pl* currFlow = getFlowPtr(flowlabel);
	currFlow->setFinishTime(Simulator::Now().GetSeconds());

	GearboxPktTag tag;
	Packet* packet = GetPointer(GetPointer(re)->GetPacket());
	packet->PeekPacketTag(tag);
	//tag.SetDequeTime(Simulator::Now().GetSeconds());
	//packet->ReplacePacketTag(tag);

	int departureround = tag.GetDepartureRound();
	Record("DequeuedPktsList.txt", departureround, re);

	// statistic per 1000 packets dequeued
	if (departureround % 1000 == 0){
		int total = levels[0].getFifoEnque() + levels[0].getFifoDeque() + levels[0].getPifoEnque() + levels[0].getPifoDeque();
		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out);
		thr <<  Simulator::Now().GetSeconds() << " TOTAL:" << total << " fifoenque:" << levels[0].getFifoEnque() << " fifodeque:" << levels[0].getFifoDeque()
			<< " pifoenque:" << levels[0].getPifoEnque() << " pifodeque:" << levels[0].getPifoDeque()
			<< " reload:"<< levels[0].getReload() << " reloadIni:" << levels[0].getReloadIni() 
			<< " uid:" << maxUid << " vt:" << currentRound 
                        << " drop:" << getDropCount() << " = dropA:" << dropCountA << " + dropB:" << dropCountB << " - dropC:" << dropCountC
			<< " flowNo:" << flowNo << " non-empty" << std::endl;
	}

	for(int i = 0; i < DEFAULT_VOLUME; i++){
		int currentfifo = levels[i].getEarliestFifo();
		if( currentfifo == -1){
			currentfifo = 0;
		}
		for (int j = 0; j < FIFO_PER_LEVEL; j++){
			int fifosize = levels[i].getFifoNPackets((j+currentfifo)%FIFO_PER_LEVEL);
				FILE *fp;
				string path = "GBResult/pktsList/Level_";
				path.append(to_string(i));
				path.append("_fifo_");
				path.append(to_string(j));
				path.append("_Record.txt");
				fp = fopen(path.data(), "a+"); //open and write
				fprintf(fp, "%d ", fifosize);
				fclose(fp);
		}
	}

	return re;
   }



   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::FifoDequeue(int earliestLevel) {
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

	return p;
   }



   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::PifoDequeue() {

   	QueueDiscItem* pifoitem = levels[0].pifoDeque();
	GearboxPktTag tag;
	pifoitem->GetPacket()->PeekPacketTag(tag);
	this->setCurrentRound(tag.GetDepartureRound());

	Ptr<QueueDiscItem> p = pifoitem;
	setPktCount(pktCount - 1);
	return p;

   }


   void Gearbox_pl_fid_flex::Reload(){

	int j = 0;
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
			
			if(levels[j].finishCurrentFifoReload()){
				levels[j].TerminateReload();
				levels[j].setRemainingQ(SPEEDUP_FACTOR);
			}
		}
    }



    void Gearbox_pl_fid_flex::setDropCount(int count){
	this->dropCount = count;
    }


    int Gearbox_pl_fid_flex::getDropCount(){
	return dropCount;
    }


    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(string fid, int fno, int weight, int burstness) {
	Flowlist.push_back(fid);

        string key = convertKeyValue(fid); 
        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight, burstness);
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));

        return this->flowMap[key];
    }



    int Gearbox_pl_fid_flex::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) {
        string key = convertKeyValue(fid); 
	// update flow info
        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // only update last packet finish time if the packet wasn't dropped
        this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));

        return 0;
    }


    string Gearbox_pl_fid_flex::convertKeyValue(string fid) { 
        stringstream ss;
        ss << fid; 
        string key = ss.str();

        return key;
    }



    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoRemove(void) {
        return 0;
    }


    Ptr<const QueueDiscItem> Gearbox_pl_fid_flex::DoPeek(void) const {
        return 0;
    }



    Gearbox_pl_fid_flex::~Gearbox_pl_fid_flex()
    {
        NS_LOG_FUNCTION(this);
    }



    TypeId Gearbox_pl_fid_flex::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::Gearbox_pl_fid_flex")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<Gearbox_pl_fid_flex>();

        return tid;
    }



    bool Gearbox_pl_fid_flex::CheckConfig(void)
    {
        NS_LOG_FUNCTION(this);
        if (GetNQueueDiscClasses() > 0){
            NS_LOG_ERROR("Gearbox_pl_fid_flex cannot have classes");
            return false;
        }

        if (GetNPacketFilters() != 0){
            NS_LOG_ERROR("Gearbox_pl_fid_flex needs no packet filter");
            return false;
        }

        if (GetNInternalQueues() > 0) {
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
	
	int dp = tag.GetDepartureRound();
	
	string path = "GBResult/pktsList/";
	path.append(fname);
	
	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "%d", dp);
	fprintf(fp, "\t");
	fclose(fp);

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
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "%d", departureRound);
	fprintf(fp, "\t");
	fclose(fp);
    }

}







	

