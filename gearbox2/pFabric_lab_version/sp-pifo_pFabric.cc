/**
* SP-PIFO:
*       q0 ======
*       q1 ======
*          ...
*   q(N-1) ======
**/

#include "ns3/Level_flex_pFabric.h"
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
#include "sp-pifo_pFabric.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-queue-disc-item.h"

using namespace std;

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("SpPifo_pFabric");
    NS_OBJECT_ENSURE_REGISTERED(SpPifo_pFabric);

    SpPifo_pFabric::SpPifo_pFabric() {
        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue");
        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));
        factory.Set("MaxPackets", UintegerValue(DEFAULT_VOLUME));

	// create fifos and assign the fifo length
	for (int i = 0; i < DEFAULT_PQ; i++) {
            fifos[i] = GetPointer(factory.Create<Queue>());
            uint32_t maxSize = DEFAULT_VOLUME;
            fifos[i]->SetMaxPackets(maxSize);
        }

	maxUid = 0;
        currentRound = 0;
	/*
	// initialize flow weights
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
	*/
	int no = 0;
	///*pfabric
	ifstream infile1;
	infile1.open("scratch/traffic_pFabric.txt");
	infile1 >> no;
	for (int j = 0; j < no; j++){
		int flowSize;
		uint32_t src, dst, dport;
		double start_time;
		//double start_time,stop_time;
		infile1 >> src >> dst >> dport >> flowSize >> start_time;
		//infile1 >> src >> dst >> dport >> maxPacketCount >> start_time>>stop_time;
		//cout<<"flow size"<<maxPacketCount<<endl;
		flowsize.push_back(flowSize);
	}

	infile1.close();
	
        factory.SetTypeId("ns3::DropTailQueue");
        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));
        factory.Set("MaxPackets", UintegerValue(DEFAULT_FIFO_N_SIZE));
        for (int i = 0; i < 10000; i++) {
            Per_flow_queues[i] = GetPointer(factory.Create<Queue>());
            uint32_t maxSize = DEFAULT_FIFO_N_SIZE;
            Per_flow_queues[i]->SetMaxPackets(maxSize);
        }


	cout << "SP-Pifo CREATED" << endl;
    }


    string SpPifo_pFabric::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	TcpHeader header;
    	GetPointer(item)->GetPacket()->PeekHeader(header);

	stringstream ss;
        ss << ipHeader.GetSource().Get() << header.GetSourcePort();
        string flowLabel = ss.str();
	return flowLabel;
    }


   string SpPifo_pFabric::convertKeyValue(string fid) { 
        stringstream ss;
        ss << fid; 
        string key = ss.str();

        return key;
   }

   Flow_pl* SpPifo_pFabric::insertNewFlowPtr(string fid, int fno, int weight, int flowsize, vector<int> packet_List,int burstness) {
	Flowlist.push_back(fid);
        string key = convertKeyValue(fid); 
        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight, burstness, flowsize);
	std::ofstream thr1 ("GBResult_pFabric/weight.dat", std::ios::out | std::ios::app);
	thr1 << fno << " " << weight << endl;
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
        return this->flowMap[key];
    }

 
   Flow_pl* SpPifo_pFabric::getFlowPtr(string flowlabel) {
        string key = convertKeyValue(flowlabel);
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    //int weight = rand() % 15 + 1;
	    int weight = DEFAULT_WEIGHT;
	    /*if (flowNo % 3 == 0){
		weight = 8;
	    }*/
	    //int weight = weights.at(flowNo);
	    int flowSize = flowsize.at(flowNo);
	    flowNo++;
	    this->enqueMap[flowNo] = 0;
	    return this->insertNewFlowPtr(flowlabel,flowNo, weight, flowSize, {}, DEFAULT_BURSTNESS);
        }
        return this->flowMap[key];
    }


    int SpPifo_pFabric::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) {
        string key = convertKeyValue(fid); 
	// update flow info
	int remainingsize = flowPtr->getRemaningSize();
	//cout<<"list size"<<pktsList.size()<<endl;
	int remaining = 0 > (remainingsize-1) ? 0 : remainingsize-1;
	flowPtr->setRemaningSize(remaining);
	this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));

        return 0;
    }


    int SpPifo_pFabric::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	int departureRound = currFlow->getRemaningSize();
	return departureRound;
    }


    bool SpPifo_pFabric::DoEnqueue(Ptr<QueueDiscItem> item) {
	
        NS_LOG_FUNCTION(this);
	
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	int index = getFlowPtr(getFlowLabel(item))->getFlowNo();
	bool enqueflow = this->enqueMap[index];
	if(enqueflow == 0){
		enque += 1;
		uid = uid + 1;
		maxUid = uid > maxUid ? uid : maxUid;

		// 1. calculate the incoming pkt's rank
		string flowlabel = getFlowLabel(item);
		Flow_pl* currFlow = getFlowPtr(flowlabel);
		int rank = RankComputation(GetPointer(item), currFlow);

		if((uid > 85365)&& (uid < 85700)){
			for (int k = 0; k < DEFAULT_PQ; k++){
				FifoPrint(k);
			}	
		}

		// add tag values, including rank and uid
		GearboxPktTag tag;
		Packet* packet = GetPointer(item->GetPacket());
		packet->PeekPacketTag(tag);
		packet->AddPacketTag(GearboxPktTag(currFlow->getFlowNo(), uid, rank));


		// 2. enque to the target queue with bottom-up comparisions
		for (int i = DEFAULT_PQ - 1; i >= 0 ; i--){
			// push up
			if (rank >= bounds[i]){
				if (this->GetQueueSize(i) < DEFAULT_VOLUME){
					// enque
					fifos[i]->Enqueue(Ptr<QueueDiscItem>(item));	

					enpkt += 1;
					size += 1; // trace size in sp-pifo
				
					std::ofstream thr ("GBResult_pFabric/SpPifo_pFabricQBounds.dat", std::ios::out | std::ios::app);
					thr << Simulator::Now().GetSeconds()<<" " ;
					for (int k = 0; k < DEFAULT_PQ; k++){
						thr << bounds[k] << " ";
					}
					thr << endl;
				
					this->Record("EnqueuedPktsList.txt", item);
					/*std::ofstream thr3 ("GBResult_pFabric/pktsList/Enque_bound8.dat", std::ios::out | std::ios::app);
					thr3 << "(" << rank - bounds[7] << "," << i << "," << currFlow->getFlowNo() << ")" << " ";
					std::ofstream thr4 ("GBResult_pFabric/pktsList/Size.dat", std::ios::out | std::ios::app);
					thr4 << "(" << i << "," << this->GetQueueSize(i) << "," << currFlow->getFlowNo() << ")" << " ";
					std::ofstream thr5 ("GBResult_pFabric/pktsList/SizePlot7.dat", std::ios::out | std::ios::app);
					thr5 << Simulator::Now().GetSeconds() << " " << this->GetQueueSize(7) << endl;
					std::ofstream thr6 ("GBResult_pFabric/pktsList/SizePlot6.dat", std::ios::out | std::ios::app);
					thr6 << Simulator::Now().GetSeconds() << " " << this->GetQueueSize(6) << endl;*/
				

					// update queue bound
					bounds[i] = rank;
					// record into the map which records all pkts who are currently in the system
					addSchePkt(uid, rank);
					// updat the flow's last finish time
					this->updateFlowPtr(rank, flowlabel, currFlow);
					this->enqueMap[index] = 1;
					//record each flow
					stringstream ss;
					ss << "Enque_"<< index<<".txt";
					string filename = ss.str();
					RecordFlow(flowlabel, rank, uid, filename );
					return true;
				}
				// fifo overflow
				else{
					Drop(item);
					drop += 1;
					std::ofstream thr1 ("GBResult_pFabric/SpPifo_pFabricdrop.dat", std::ios::out | std::ios::app);
					thr1 << Simulator::Now().GetSeconds() << " " << drop << endl;
					return false;
				}
			}
			else{
				// push down
				if (i == 0){
					if (this->GetQueueSize(i) < DEFAULT_VOLUME){
						// enque
						fifos[i]->Enqueue(Ptr<QueueDiscItem>(item));
						// record into the map which records all pkts who are currently in the system
						addSchePkt(uid, rank);
						// updat the flow's last finish time
						this->updateFlowPtr(rank, flowlabel, currFlow);
						this->enqueMap[index] = 1;
						//record each flow
						stringstream ss;
						ss << "Enque_"<< index<<".txt";
						string filename = ss.str();
						RecordFlow(flowlabel, rank, uid, filename );

						// update all queue bounds
						int cost = bounds[i] - rank;
						bounds[i] = rank;
						for (int j = 1; j < DEFAULT_PQ; j++){
							bounds[j] -= cost;
						}

						size += 1;

						std::ofstream thr ("GBResult_pFabric/SpPifo_pFabricQBounds.dat", std::ios::out | std::ios::app);
						for (int k = 0; k < DEFAULT_PQ; k++){
							thr << bounds[k] << " ";
						}
						thr << endl;
						this->Record("EnqueuedPktsList.txt", item);

						/*std::ofstream thr3 ("GBResult_pFabric/pktsList/Enque_bound8.dat", std::ios::out | std::ios::app);
						thr3 << Simulator::Now().GetSeconds() << rank - bounds[7] << endl;*/
						return true;
					}
					// fifo overflow
					else{
						Drop(item);
						drop += 1;
						std::ofstream thr1 ("GBResult_pFabric/SpPifo_pFabricdrop.dat", std::ios::out | std::ios::app);
						thr1 << Simulator::Now().GetSeconds() << " " << drop << endl;
						return false;
					}	
				}
			}
		}
		// updat the flow's last finish time
		return true;
	}
	else{
		if ((Per_flow_queues[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
			Per_flow_queues[index]->Enqueue(Ptr<QueueDiscItem>(item));
			return true;
		}
		else{
			Drop(item);
			drop_perQ += 1;
			return false;
		}
	}
	

    }


    Ptr<QueueDiscItem> SpPifo_pFabric::DoDequeue() {
	deque += 1;

        if (size <= 0) {
	    empty += 1;
            return 0;
        }
	Ptr<QueueDiscItem> item;
	// deque from the highest priority non-empty fifo
	for (int i = 0; i < DEFAULT_PQ; i++){
		if (this->GetQueueSize(i) > 0){	
			item = StaticCast<QueueDiscItem>(fifos[i]->Dequeue());
			depkt += 1;
			this->Record("DequeuedPktsList.txt", item);
			size -= 1;

			// update vt to be the pkt's rank
			GearboxPktTag tag;
			GetPointer(item->GetPacket())->PeekPacketTag(tag);
			currentRound = tag.GetDepartureRound();
			break;
		}
	}
	GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	if(item != NULL){
		removeSchePkt(tag.GetUid());
		int inversion = cal_inversion_mag(tag.GetDepartureRound());
		if (inversion > 0){
			inv_count += 1;
			inv_mag += inversion;
		}
		std::ofstream thr2 ("GBResult_pFabric/inversion_record.dat", std::ios::out);
		thr2 << "count: " << inv_count << "  magnitude: " << inv_mag << endl;
		std::ofstream thr ("GBResult_pFabric/CountStat.dat", std::ios::out);
		thr <<  Simulator::Now().GetSeconds() << " enque:" << enque << " deque:" << deque << " drop:" << drop <<" drop_perQ:" << drop_perQ  << " flowNo:" << flowNo << std::endl;

		//cout<<"deque rank"<<currentRound<<endl;
		string flowlabel = getFlowLabel(item);
		Flow_pl* currFlow = getFlowPtr(flowlabel);
		currFlow->setFinishTime(Simulator::Now().GetSeconds());
		///*pFarbric
		//clear the enque tag for re's flow
		int index = getFlowPtr(getFlowLabel(item))->getFlowNo();
		this->enqueMap[index] = 0;
		//reload the pkt in perflow queue to scheduler
		if (!Per_flow_queues[index]->IsEmpty()) {
		    Ptr<QueueDiscItem> item1 = StaticCast<QueueDiscItem>(Per_flow_queues[index]->Dequeue());
		    DoEnqueue(Ptr<QueueDiscItem>(item1));
		}

		int remainingsize =  currFlow->getRemaningSize();
		//cout<<"dodeque remaining"<<remainingsize<<endl;
		if(remainingsize == 0){
			double FCT = currFlow->getFinishTime() - currFlow->getStartTime();
			RecordFCT("FCT.txt", index,FCT);
		}
		//*/
	}
	this->Record("DequeuedPktsList.txt", item);
	
	stringstream ss;
	ss << "Deque_"<< getFlowPtr(getFlowLabel(item))->getFlowNo()<<".txt";
	string filename = ss.str();
	RecordFlow(getFlowLabel(item), tag.GetDepartureRound(),tag.GetUid(), filename );

	
	return item;
    }


    void SpPifo_pFabric::Record(string fname, Ptr<QueueDiscItem> item){
	GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	int dp = tag.GetDepartureRound();
	
	string path = "GBResult_pFabric/pktsList/";
	path.append(fname);
	
	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	/*fprintf(fp, "%d ", enque);
	fprintf(fp, "%d ", enpkt);
	fprintf(fp, "%d ", deque);
	fprintf(fp, "%d ", empty);
	fprintf(fp, "%d ", depkt);
	fprintf(fp, "(");
	fprintf(fp, "%f", Simulator::Now().GetSeconds());
	fprintf(fp, ",");*/
	//fprintf(fp, "%d", dp);
	/*fprintf(fp, ",");
	fprintf(fp, "%d", currentRound);
	fprintf(fp, ",");
	fprintf(fp, "%d", dp - currentRound);
	fprintf(fp, ")");*/
	//fprintf(fp, "\t");
	//fprintf(fp, "%s", "("); 
	fprintf(fp, "%d", dp);
	/*fprintf(fp, "%s", ","); 
	fprintf(fp, "%d", tag.GetUid());
	fprintf(fp, "%s", ",");
	fprintf(fp, "%f", Simulator::Now().GetSeconds());  
	fprintf(fp, "%s", ")");*/
	fprintf(fp, "\t");

	fclose(fp);
   }


    Ptr<QueueDiscItem> SpPifo_pFabric::DoRemove(void) {
        return 0;
    }


    Ptr<const QueueDiscItem> SpPifo_pFabric::DoPeek(void) const {
        return 0;
    }


    int SpPifo_pFabric::GetQueueSize(int index) {
	return fifos[index]->GetNPackets();
    }

    SpPifo_pFabric::~SpPifo_pFabric()
    {
        NS_LOG_FUNCTION(this);
    }


    TypeId SpPifo_pFabric::GetTypeId(void){
        static TypeId tid = TypeId("ns3::SpPifo_pFabric")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<SpPifo_pFabric>()
            ;

        return tid;
    }


    void SpPifo_pFabric::InitializeParams(void){
        NS_LOG_FUNCTION(this);
    }


    bool SpPifo_pFabric::CheckConfig(void){
        NS_LOG_FUNCTION(this);

        if (GetNQueueDiscClasses() > 0){
            NS_LOG_ERROR("SpPifo_pFabric cannot have classes");
            return false;
        }
        if (GetNPacketFilters() != 0){
            NS_LOG_ERROR("SpPifo_pFabric needs no packet filter");
            return false;
        }
        if (GetNInternalQueues() > 0){
            NS_LOG_ERROR("SpPifo_pFabric cannot have internal queues");
            return false;
        }

        return true;
    }


    void SpPifo_pFabric::FifoPrint(int index){
	cout << "q" << index << " : b" << bounds[index];
	for (int i = 0; i < this->GetQueueSize(index); i++)
  	{	
		Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[index]->Dequeue());
    		Packet* packet = GetPointer(item->GetPacket());
	 	GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		cout << " (" << i << ", " << tag.GetDepartureRound() << ", " << tag.GetUid() << "," <<getFlowPtr(getFlowLabel(item))->getFlowNo() << ") ";
		fifos[index]->Enqueue(item);
  	}
	cout << endl;
    }

   void SpPifo_pFabric::RecordFlow(string flowlabel, int departureRound, int uid,string filename){
	string path = "GBResult_pFabric/pktsList/";
	path.append(filename);

	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "(%d,", departureRound);
	fprintf(fp, "%d,", uid);
	fprintf(fp, "%f,", Simulator::Now().GetSeconds());
	fprintf(fp, "%d),", getFlowPtr(flowlabel)->getFlowNo());
	fprintf(fp, "\t");
	fclose(fp);
    }


    void SpPifo_pFabric::RecordFCT(string filename, int index,double FCT){
	string path = "GBResult_pFabric/";
	path.append(filename);

	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "%d\t", index);
	fprintf(fp, "%f", FCT);
	fprintf(fp, "\n");
	fclose(fp);
    }

    int SpPifo_pFabric::addSchePkt(int uid, int departureRound){
	qpkts[uid] = departureRound;
	return qpkts.size();
    }

    int SpPifo_pFabric::removeSchePkt(int uid){
	if (0 == qpkts.erase(uid)){
		cout << "cannot erase from map" << endl;
	}
	return qpkts.size();
    }


    int SpPifo_pFabric::cal_inversion_mag(int dp){
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







	
