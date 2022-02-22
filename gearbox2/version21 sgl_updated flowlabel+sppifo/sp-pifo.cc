/**
* SP-PIFO:
*       q0 ======
*       q1 ======
*          ...
*   q(N-1) ======
**/

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
#include "sp-pifo.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-queue-disc-item.h"

using namespace std;

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("SpPifo");
    NS_OBJECT_ENSURE_REGISTERED(SpPifo);

    SpPifo::SpPifo() {
        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue");
        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));
        factory.Set("MaxPackets", UintegerValue(DEFAULT_PQ));

	// create fifos and assign the fifo length
	for (int i = 0; i < DEFAULT_PQ; i++) {
            fifos[i] = GetPointer(factory.Create<Queue>());
            uint32_t maxSize = DEFAULT_VOLUME;
            fifos[i]->SetMaxPackets(maxSize);
        }

	maxUid = 0;
        currentRound = 0;

	cout << "SP-Pifo CREATED" << endl;
    }


    string SpPifo::getFlowLabel(Ptr<QueueDiscItem> item){
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


   string SpPifo::convertKeyValue(string fid) { 
        stringstream ss;
        ss << fid; 
        string key = ss.str();

        return key;
   }

   Flow_pl* SpPifo::insertNewFlowPtr(string fid, int fno, int weight, int burstness) {
	Flowlist.push_back(fid);
        string key = convertKeyValue(fid); 
        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight, burstness);
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
        return this->flowMap[key];
    }

 
   Flow_pl* SpPifo::getFlowPtr(string flowlabel) {
        string key = convertKeyValue(flowlabel);
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    int weight = DEFAULT_WEIGHT;
	    if (flowNo % 3 == 0){
		weight = 8;
	    }
	    flowNo++;
	    cout << "flowNo:" << flowNo << endl;
	    return this->insertNewFlowPtr(flowlabel, flowNo, weight, DEFAULT_BURSTNESS);
        }
        return this->flowMap[key];
    }


    int SpPifo::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) {
        string key = convertKeyValue(fid); 
	// update flow info
        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // only update last packet finish time if the packet wasn't dropped
        this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));

        return 0;
    }


    int SpPifo::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	int curLastFinishRound = currFlow->getLastFinishRound();
	//cout << "currentRound:" << currentRound << " curLastFinishRound:" << curLastFinishRound << endl;
	return max(currentRound, curLastFinishRound);
    }


    bool SpPifo::DoEnqueue(Ptr<QueueDiscItem> item) {
	//cout << "\n======enque======" << endl;
        NS_LOG_FUNCTION(this);
	
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();

	uid = uid + 1;
	maxUid = uid > maxUid ? uid : maxUid;

	// 1. calculate the incoming pkt's rank
	string flowlabel = getFlowLabel(item);
        Flow_pl* currFlow = getFlowPtr(flowlabel);
	//cout << "currFlow.getFlowNo():" << currFlow->getFlowNo() << endl;
	int rank = RankComputation(GetPointer(item), currFlow);


	/*====test
	for (int i = 0; i < DEFAULT_PQ; i++){
		//cout << "size:" << this->GetQueueSize(i) << endl;
		this->FifoPrint(i);
	}
	cout << "rank:" << rank << endl;*/
	

	// drop the pkt if its rank is larger than the available rank range
	if (rank - currentRound > RANK_RANGE){
		Drop(item);
		drop += 1;
		std::ofstream thr1 ("GBResult/SpPifodrop.dat", std::ios::out | std::ios::app);
		thr1 << Simulator::Now().GetSeconds() << " " << drop << endl;
		return false;
	}

	// add tag values, including rank and uid
	GearboxPktTag tag;
	Packet* packet = GetPointer(item->GetPacket());
	packet->PeekPacketTag(tag);
	float time = Simulator::Now().GetSeconds();
	packet->AddPacketTag(GearboxPktTag(rank, time, uid));

	// updat the flow's last finish time
	this->updateFlowPtr(rank, flowlabel, currFlow);

	// 2. enque to the target queue with bottom-up comparisions
	for (int i = DEFAULT_PQ - 1; i >= 0 ; i--){
		// push up
		if (rank >= bounds[i]){
			if (this->GetQueueSize(i) < DEFAULT_VOLUME){
				// enque
				fifos[i]->Enqueue(Ptr<QueueDiscItem>(item));
				// update queue bound
				bounds[i] = rank;
				
				size += 1; // trace size in sp-pifo
				
				std::ofstream thr ("GBResult/SpPifoQBounds.dat", std::ios::out | std::ios::app);
				for (int k = 0; k < DEFAULT_PQ; k++){
					thr << bounds[k] << " ";
				}
				thr << endl;
				
				this->Record("EnqueuedPktsList.txt", item);
				std::ofstream thr3 ("GBResult/pktsList/Enque_bound8.dat", std::ios::out | std::ios::app);
				thr3 << "(" << rank - bounds[7] << "," << i << ")" << " ";
				/*====test
				for (int i = 0; i < DEFAULT_PQ; i++){
					//cout << "size:" << this->GetQueueSize(i) << endl;
					this->FifoPrint(i);
				}*/
				return true;
			}
			// fifo overflow
			else{
				Drop(item);
				drop += 1;
				std::ofstream thr1 ("GBResult/SpPifodrop.dat", std::ios::out | std::ios::app);
				thr1 << Simulator::Now().GetSeconds() << " " << drop << endl;
			}
		}
		else{
			// push down
			if (i == 0){
				if (this->GetQueueSize(i) < DEFAULT_VOLUME){
					// enque
					fifos[i]->Enqueue(Ptr<QueueDiscItem>(item));
					// update all queue bounds
					int cost = bounds[i] - rank;
					bounds[i] = rank;
					for (int j = 1; j < DEFAULT_PQ; j++){
						bounds[j] -= cost;
					}

					size -= 1;

					std::ofstream thr ("GBResult/SpPifoQBounds.dat", std::ios::out | std::ios::app);
					for (int k = 0; k < DEFAULT_PQ; k++){
						thr << bounds[k] << " ";
					}
					thr << endl;
					this->Record("EnqueuedPktsList.txt", item);
					std::ofstream thr3 ("GBResult/pktsList/Enque_bound8.dat", std::ios::out | std::ios::app);
					thr3 << "(" << rank - bounds[7] << "," << i << ")" << " ";
				}
				// fifo overflow
				else{
					Drop(item);
					drop += 1;
					std::ofstream thr1 ("GBResult/SpPifodrop.dat", std::ios::out | std::ios::app);
					thr1 << Simulator::Now().GetSeconds() << " " << drop << endl;
				}	
			}
			else{
				continue;
			}
		}
	}

	/*====test
	for (int i = 0; i < DEFAULT_PQ; i++){
		//cout << "size:" << this->GetQueueSize(i) << endl;
		this->FifoPrint(i);
	}*/

	return true;
    }


    Ptr<QueueDiscItem> SpPifo::DoDequeue() {
	//cout << "\n======Deque" << endl;

        if (size <= 0) {
            return 0;
        }
	Ptr<QueueDiscItem> item;
	// deque from the highest priority non-empty fifo
	for (int i = 0; i < DEFAULT_PQ; i++){
		if (this->GetQueueSize(i) != 0){
			item = StaticCast<QueueDiscItem>(fifos[i]->Dequeue());

			// update vt to be the pkt's rank
			GearboxPktTag tag;
			GetPointer(item->GetPacket())->PeekPacketTag(tag);
			currentRound = tag.GetDepartureRound();
		}
	}
	size -= 1;
	/*std::ofstream thr ("GBResult/SpPifosize.dat", std::ios::out | std::ios::app);
	thr << size << " ";*/

	if (item != NULL){
		this->Record("DequeuedPktsList.txt", item);
	}

	/*====test
	for (int i = 0; i < DEFAULT_PQ; i++){
		this->FifoPrint(i);
	}*/

	return item;
    }


    void SpPifo::Record(string fname, Ptr<QueueDiscItem> item){
	GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	int dp = tag.GetDepartureRound();
	
	string path = "GBResult/pktsList/";
	path.append(fname);
	
	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "(");
	fprintf(fp, "%d", dp);
	fprintf(fp, ",");
	fprintf(fp, "%d", currentRound);
	fprintf(fp, ")");
	fprintf(fp, "\t");
	fclose(fp);
   }


    Ptr<QueueDiscItem> SpPifo::DoRemove(void) {
        return 0;
    }


    Ptr<const QueueDiscItem> SpPifo::DoPeek(void) const {
        return 0;
    }


    int SpPifo::GetQueueSize(int index) {
	return fifos[index]->GetNPackets();
    }

    SpPifo::~SpPifo()
    {
        NS_LOG_FUNCTION(this);
    }


    TypeId SpPifo::GetTypeId(void){
        static TypeId tid = TypeId("ns3::SpPifo")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<SpPifo>()
            ;

        return tid;
    }


    void SpPifo::InitializeParams(void){
        NS_LOG_FUNCTION(this);
    }


    bool SpPifo::CheckConfig(void){
        NS_LOG_FUNCTION(this);

        if (GetNQueueDiscClasses() > 0){
            NS_LOG_ERROR("SpPifo cannot have classes");
            return false;
        }
        if (GetNPacketFilters() != 0){
            NS_LOG_ERROR("SpPifo needs no packet filter");
            return false;
        }
        if (GetNInternalQueues() > 0){
            NS_LOG_ERROR("SpPifo cannot have internal queues");
            return false;
        }

        return true;
    }


    void SpPifo::FifoPrint(int index){
	cout << "q" << index << " : b" << bounds[index];
	for (int i = 0; i < this->GetQueueSize(index); i++)
  	{	
		Ptr<QueueItem> item = fifos[index]->Dequeue();
    		Packet* packet = GetPointer(item->GetPacket());
	 	GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		cout << " (" << i << ", " << tag.GetDepartureRound() << ", " << tag.GetUid() << ") ";
		fifos[index]->Enqueue(item);
  	}
	cout << endl;
    }

}







	

