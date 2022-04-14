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



#include "Fifo.h"



#include "ns3/queue.h"



#include "ns3/queue-disc.h"



#include "ns3/log.h"



#include "ns3/simulator.h"



#include "ns3/ipv4-queue-disc-item.h"







using namespace std;



namespace ns3 {







    NS_LOG_COMPONENT_DEFINE("Fifo");



    NS_OBJECT_ENSURE_REGISTERED(Fifo);







    Fifo::Fifo() {

        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue");
        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));
        factory.Set("MaxPackets", UintegerValue(DEFAULT_VOLUME));
	fifo = GetPointer(factory.Create<Queue>());
	fifo->SetMaxPackets(DEFAULT_VOLUME);

	size = 0;
	maxUid = 0;
        currentRound = 0;
	//this->pifo = Pifo(H_value, H_value);

	ifstream infile;
	infile.open("scratch/weights.txt");
	int no = 0;
	infile >> no;
        for (int i = 0; i < no; i++){
		int temp = 0;
		infile >> temp;
		weights.push_back(temp);
	}
	infile.close();

	cout << "FIFO CREATED" << endl;
    }





    bool Fifo::DoEnqueue(Ptr<QueueDiscItem> item) {


        NS_LOG_FUNCTION(this);


	// 1. calculate the incoming pkt's rank
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	string flowlabel = getFlowLabel(item);
        Flow_pl* currFlow = getFlowPtr(flowlabel);
	int rank = RankComputation(GetPointer(item), currFlow);


	// 2. add tag values, including rank and uid
	GearboxPktTag tag;
	Packet* packet = GetPointer(item->GetPacket());
	packet->PeekPacketTag(tag);
	uid = uid + 1;
	packet->AddPacketTag(GearboxPktTag(currFlow->getFlowNo(), uid, rank));

	
	// 3. enque
	if (size < DEFAULT_VOLUME){
		fifo->Enqueue(item);
		//pifo.Push(GetPointer(item), rank);

		// 3. updat the flow's last finish time
		this->updateFlowPtr(rank, flowlabel, currFlow);
		// record into the map which records all pkts who are currently in the system
		addSchePkt(uid, rank);
		
		size += 1;
		std::ofstream thr ("GBResult/Fifosize.dat", std::ios::out | std::ios::app);
		thr << Simulator::Now().GetSeconds() << " " << size << endl;
		return true;
	}
	else{
		Drop(item);
		drop += 1;
		std::ofstream thr ("GBResult/Fifosize.dat", std::ios::out | std::ios::app);
		thr << Simulator::Now().GetSeconds() << " " << size << endl;
		std::ofstream thr1 ("GBResult/Fifodrop.dat", std::ios::out | std::ios::app);
		thr1 << Simulator::Now().GetSeconds() << " " << drop << endl;
		return false;
	}

    }



    string Fifo::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	TcpHeader header;
    	GetPointer(item)->GetPacket()->PeekHeader(header);

	stringstream ss;
        ss << ipHeader.GetSource().Get() << header.GetSourcePort();
        string flowLabel = ss.str();
	//cout << ipHeader.GetSource().Get() << " " << header.GetSourcePort() << " flowLabel:" << flowLabel << endl;
	return flowLabel;
    }


   string Fifo::convertKeyValue(string fid) { 
        stringstream ss;
        ss << fid; 
        string key = ss.str();

        return key;
   }

   Flow_pl* Fifo::insertNewFlowPtr(string fid, int fno, int weight, int burstness) {
	Flowlist.push_back(fid);
        string key = convertKeyValue(fid); 
        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight, burstness);
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
        return this->flowMap[key];
    }

 
   Flow_pl* Fifo::getFlowPtr(string flowlabel) {
        string key = convertKeyValue(flowlabel);
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    int weight = weights.at(flowNo);
	    flowNo++;
	    return this->insertNewFlowPtr(flowlabel, flowNo, weight, DEFAULT_BURSTNESS);
        }
        return this->flowMap[key];
    }


    int Fifo::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) {
        string key = convertKeyValue(fid); 
	// update flow info
        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // only update last packet finish time if the packet wasn't dropped
        this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));

        return 0;
    }


    int Fifo::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	int curLastFinishRound = currFlow->getLastFinishRound();
	//cout << "currentRound:" << currentRound << " curLastFinishRound:" << curLastFinishRound << endl;
	return max(currentRound, curLastFinishRound);
    }



    Ptr<QueueDiscItem> Fifo::DoDequeue() {
        if (size <= 0) {
            return 0;
        }
        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifo->Dequeue());
	//Ptr<QueueDiscItem> item = pifo.Pop();

	// update vt to be the pkt's rank
	GearboxPktTag tag;
	GetPointer(item->GetPacket())->PeekPacketTag(tag);
	currentRound = tag.GetDepartureRound();

	if (item != NULL){
		this->Record("DequeuedPktsList.txt", item);
	}

	size -= 1;
	std::ofstream thr ("GBResult/Fifosize.dat", std::ios::out | std::ios::app);
	thr << Simulator::Now().GetSeconds() << " " << size << endl;

	// update the map which records all pkts currently in the scheduler
	removeSchePkt(tag.GetUid());
	int inversion = cal_inversion_mag(currentRound);
	if (inversion > 0){
		inv_count += 1;
		inv_mag += inversion;
	}
	std::ofstream thr2 ("GBResult/inversion_record.dat", std::ios::out);
	thr2 << "count: " << inv_count << "  magnitude: " << inv_mag << endl;

	std::ofstream thr3 ("GBResult/CountStat.dat", std::ios::out);
	thr3 <<  Simulator::Now().GetSeconds() << " drop:" << drop << " flowNo:" << flowNo << std::endl;

	return item;
   }


   void Fifo::Record(string fname, Ptr<QueueDiscItem> item){
	GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	int dp = tag.GetDepartureRound();
	
	string path = "GBResult/pktsList/";
	path.append(fname);
	
	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	//fprintf(fp, "(");
	fprintf(fp, "%d", dp);
	//fprintf(fp, ",");
	//fprintf(fp, "%d", currentRound);
	//fprintf(fp, ")");
	fprintf(fp, "\t");
	fclose(fp);
     }


    Ptr<QueueDiscItem> Fifo::DoRemove(void) {
        //NS_LOG_FUNCTION (this);
        return 0;
    }


    Ptr<const QueueDiscItem> Fifo::DoPeek(void) const {
        //NS_LOG_FUNCTION (this);
        return 0;
    }


    Fifo::~Fifo()
    {
        NS_LOG_FUNCTION(this);
    }


    TypeId Fifo::GetTypeId(void)

    {
        static TypeId tid = TypeId("ns3::Fifo")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<Fifo>()

            ;

        return tid;

    }



    bool Fifo::CheckConfig(void)
    {

        NS_LOG_FUNCTION(this);

        if (GetNQueueDiscClasses() > 0){
            NS_LOG_ERROR("Fifo cannot have classes");
            return false;
        }

        if (GetNPacketFilters() != 0){
            NS_LOG_ERROR("Fifo needs no packet filter");
            return false;
        }
        if (GetNInternalQueues() > 0){
            NS_LOG_ERROR("Fifo cannot have internal queues");
            return false;
        }
        return true;
    }



    void Fifo::InitializeParams(void){
        NS_LOG_FUNCTION(this);
    }

    int Fifo::addSchePkt(int uid, int departureRound){
	qpkts[uid] = departureRound;
	return qpkts.size();
    }

    int Fifo::removeSchePkt(int uid){
	if (0 == qpkts.erase(uid)){
		cout << "cannot erase from map" << endl;
	}
	return qpkts.size();
    }


    int Fifo::cal_inversion_mag(int dp){
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







	

