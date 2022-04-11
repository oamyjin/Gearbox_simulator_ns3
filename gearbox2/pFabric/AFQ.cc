#include "ns3/Level_afq.h"
#include "ns3/Flow_pl.h"
#include "AFQ.h"
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <iterator>
#include <map>
#include "ns3/ipv4-header.h"
#include "ns3/ppp-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ptr.h"

#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/gearbox-pkt-tag.h"
using namespace std;
namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("AFQ");
    NS_OBJECT_ENSURE_REGISTERED(AFQ);

    AFQ::AFQ() :AFQ(DEFAULT_VOLUME) {
    }

    AFQ::AFQ(int volume) {
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
	
	///*pfabric
	ifstream infile1;
	infile1.open("scratch/traffic.txt");
	infile1 >> no;
	for (int j = 0; j < no; j++){
		uint32_t src, dst, maxPacketCount, dport;
		double start_time;
		//double start_time,stop_time;
		infile1 >> src >> dst >> dport >> maxPacketCount >> start_time;
		//infile1 >> src >> dst >> dport >> maxPacketCount >> start_time>>stop_time;
		flowsize.push_back(maxPacketCount);
	}

	infile1.close();

	ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue");
        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));
        factory.Set("MaxPackets", UintegerValue(DEFAULT_FIFO_N_SIZE));
        for (int i = 0; i < 10000; i++) {
            Per_flow_queues[i] = GetPointer(factory.Create<Queue>());
            uint32_t maxSize = DEFAULT_FIFO_N_SIZE;
            Per_flow_queues[i]->SetMaxPackets(maxSize);
        }
	//*/

    }

    void AFQ::setCurrentRound(int currentRound) {
	//cout<<"currentRound"<<currentRound<<endl;
	if(this->currentRound <= currentRound){
		this->currentRound = currentRound;
	}

	int level_currentFifo = currentRound/GRANULARITY_PER_FIFO % FIFO_PER_LEVEL;
	levels[0].setCurrentIndex(level_currentFifo);
	//cout<<"dp= "<< currentRound<<"setcurrent index = "<<level_currentFifo<<endl;
    }
    

    void AFQ::setPktCount(int pktCount) {
        this->pktCount = pktCount;
    }


    int AFQ::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	/*SFQ
    	int curLastDepartureRound = currFlow->getLastFinishRound();
    	int curStartRound = max(currentRound, curLastDepartureRound);
	return curStartRound;
	*/
	///*pFabric
	int departureRound = currFlow->getRemaningSize();	
	return departureRound;
	//*/
	
    }


    string AFQ::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	TcpHeader header;
    	GetPointer(item)->GetPacket()->PeekHeader(header);

	stringstream ss;
        ss << ipHeader.GetSource ().Get ()<<header.GetSourcePort()<<header.GetDestinationPort();  // Peixuan 04212020
        string flowLabel = ss.str();
	//cout<<"flowlabel"<<flowLabel<<endl;
	return flowLabel;
    }


    Flow_pl* AFQ::getFlowPtr(string flowlabel) {

	// insert new flows with assigned weight
        if (flowMap.find(flowlabel) == flowMap.end()) {
	    //int weight = rand()%15+1;
	    int weight = DEFAULT_WEIGHT;
	    //int weight = weights.at(flowNo);
	    ///*pFabric
	    uint32_t flowSize = flowsize.at(flowNo);
	    //*/
	    flowNo++;
	    ///*pFabric
	    this->enqueMap[flowNo] = 0;
	    return this->insertNewFlowPtr(flowlabel,flowNo, weight, flowSize, DEFAULT_BURSTNESS);
	    //*/
	    /*SFQ
	    return this->insertNewFlowPtr(flowlabel,flowNo, weight, 1, DEFAULT_BURSTNESS);
	    */
        }
        return this->flowMap[flowlabel];
    }
    bool AFQ::DoEnqueue(Ptr<QueueDiscItem> item) {
	enque += 1;
	
        NS_LOG_FUNCTION(this);
	Packet* packet = GetPointer(GetPointer(item)->GetPacket());
	
        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	int departureRound;
	bool result;
	///*pFabric
	int index = getFlowPtr(getFlowLabel(item))->getFlowNo();
	bool enqueflow = this->enqueMap[index];
	//*/
	/*SFQ
	bool enqueflow = 0;
	*/
        //cout<<"@"<<enqueflow<<endl;
	if(enqueflow == 0){
		uid = uid +1;
		string flowlabel = getFlowLabel(item);
		Flow_pl* currFlow = getFlowPtr(flowlabel);

		// calculate rank value, which is the value of departure round in this case	
		departureRound = RankComputation(GetPointer(item), currFlow);
		
		stringstream ss;
		ss << "Enque_"<< getFlowPtr(getFlowLabel(item))->getFlowNo()<<".txt";
		string filename = ss.str();
		RecordFlow(flowlabel, departureRound,uid,filename);

		//std::ofstream thr ("GBResult/pktsList/rankVtDifference.dat", std::ios::out | std::ios::app);
		//thr << departureRound - currentRound << " ";

		if ((departureRound - currentRound ) >= FIFO_PER_LEVEL * GRANULARITY_PER_FIFO) {
			////cout << " DROP!!! TOO LARGE DEPARTURE_ROUND!  departureRound:"  << departureRound << " currentRound:" << currentRound << endl;
			std::ofstream thr10 ("GBResult/pktsList/drop_pkts.dat", std::ios::out | std::ios::app);
			thr10 << departureRound <<"_flow "<< getFlowPtr(getFlowLabel(item))->getFlowNo()  <<endl;
		    	Drop(item);

			drop += 1;
			
			std::ofstream thr3 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
			thr3 << Simulator::Now().GetSeconds() << " "  << 0 << endl;
			dropCount = dropCount+1;
			dropCountA = dropCountA+1;
			dropCount = dropCount+1;
	
			this->setDropCount(dropCount);
			return false;   // 07072019 Peixuan: exceeds the maximum round
		}
		int curBrustness = currFlow->getBurstness();
		if ((departureRound - currentRound) >= curBrustness) {
			////cout << " DROP!!! TOO BURST!  departureRound:"  << departureRound << " currentRound:" << currentRound << " curBrustness:" << curBrustness << endl;
			std::ofstream thr9 ("GBResult/pktsList/drop_pkts.dat", std::ios::out | std::ios::app);
			thr9 << departureRound <<"_flow "<< getFlowPtr(getFlowLabel(item))->getFlowNo()  <<endl;
		    	Drop(item);

			drop += 1;
			std::ofstream thr3 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
			thr3 << Simulator::Now().GetSeconds() << " "  << 0 << endl;
			dropCount = dropCount+1;
			dropCountA = dropCountA+1;
			dropCount = dropCount+1;
			this->setDropCount(dropCount);
			return false;   // 07102019 Peixuan: exceeds the maximum brustness
		}		
		this->updateFlowPtr(departureRound, flowlabel, currFlow); 
		int flowid = currFlow->getFlowNo();
		result = LevelEnqueue(GetPointer(item),flowid, departureRound, uid);
		if (result == true){
			//stringstream ss;
			//ss << flowlabel<<"EnqueuedPktsList.txt";
			//string filename = ss.str();
			//Record(filename, departureRound, item);
			//Record("EnqueuedPktsList.txt", departureRound,item);
		    ////cout << "The pkt is enqueued successfully" << endl;
		    ////cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;
		    ///*pFabric
		    this->enqueMap[index] = 1;
		    //*/
/*
		    cout<<"!"<<endl;
		    for (int fifo = 0;fifo<25;fifo++){
			 levels[0].fifoPrint(fifo);
		    }*/
			setPktCount(pktCount + 1);
		    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque " << packet);
		    return true;
		}
		else{
		// fifo overflow drop pkt
		    ////cout << "The pkt fail to be enqueued" << endl;
		    ////cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;
		    return false;
		}
	}
	else{
		///*pFabric
		//enqueue into per flow queue
		if ((Per_flow_queues[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
			Per_flow_queues[index]->Enqueue(Ptr<QueueDiscItem>(item));	
			//cout<<"!per flow queue"<<endl;
		}
		else{
			Drop(item);
			//cout<<"drop per flow queue: "<<index<<endl;
		}
		//*/
		return true;
		
	}
    }
    bool AFQ::LevelEnqueue(QueueDiscItem* item, int flowid, int departureRound, int uid){
	bool result;
	//if there is inversion, for example, enqueued pkt is smaller than vt, enqueue to current fifo in level 0
	AddTag(flowid, departureRound, uid, item);
	///*SFQ
	result = FifoEnqueue(item, departureRound/GRANULARITY_PER_FIFO % FIFO_PER_LEVEL, 0);
	//*/
	/*pFabric
	result = FifoEnqueue(item, 0, 0);
	
	*/
	return result;
    }

   bool AFQ::FifoEnqueue(QueueDiscItem* item, int index, int level){
	////cout<<endl;
	////cout<<"Gearbox FifoEnqueue <<<<<<<<<<<<<<<<Start"<<endl;
	Packet* packet = GetPointer(item->GetPacket());
	GearboxPktTag tag;
	packet->PeekPacketTag(tag);
	QueueDiscItem* re = levels[level].fifoEnque(item, index, tag.GetFlowNo());
	if(re != 0){//?if(re != 0)
		
		Drop(re);
		drop += 1;
		std::ofstream thr5 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
		thr5 << Simulator::Now().GetSeconds() << " "  << 0 << endl;
		std::ofstream thr6 ("GBResult/pktsList/Level0_fifo_drop.dat", std::ios::out | std::ios::app);
		thr6 << (index - levels[0].getEarliestFifo() + FIFO_PER_LEVEL) % FIFO_PER_LEVEL << " ";
		dropCount = dropCount+1;
		dropCountB = dropCountB+1;
		this->setDropCount(dropCount);
		return false;
	}
	return true;
    }
    void AFQ::AddTag(int flowid, int departureRound, int uid, QueueDiscItem* item){
	GearboxPktTag tag;
	Packet* packet = GetPointer(item->GetPacket());
	packet->PeekPacketTag(tag);
	packet->AddPacketTag(GearboxPktTag( departureRound, Simulator::Now().GetSeconds(),uid ));
    }
    int AFQ::cal_index(int level, int departureRound){
	int term = 1;
	
	for (int i = 0; i < level; i++){
 
		term *= FIFO_PER_LEVEL;
		
	}
	//////cout << "CalculateIndex:" << departureRound / term % FIFO_PER_LEVEL << endl;
	
	return departureRound / term % FIFO_PER_LEVEL;
    }
    Ptr<QueueDiscItem> AFQ::DoDequeue() {
	deque += 1;
	////cout<<endl;
	////cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;
	//int earliestLevel = findearliestpacket(DEFAULT_VOLUME);
	std::ofstream thr ("GBResult/CountStat.dat", std::ios::out | std::ios::app);
	if(levels[0].isAllFifosEmpty()){
		/*SFQ
		FlowMap::iterator it;
		int i = 1;
		for (it = flowMap.begin(); it != flowMap.end(); it++) {
			string s = it->first;
			double FCT = flowMap[s]->getFinishTime() - flowMap[s]->getStartTime();
			RecordFCT("FCT.txt", flowMap[s]->getFlowNo(),FCT);
			cout<<"%"<<flowMap[s]->getFlowNo()<<endl;
			i++;

		}
		*/
		enqueCount = 0;
		dequeCount = 0;
		for(int i = 0; i<DEFAULT_VOLUME; i++){
			
			enqueCount += levels[0].getFifoEnque();
			dequeCount += levels[0].getFifoDeque();
			
		}

		thr <<  Simulator::Now().GetSeconds() <<" uid "<<uid<< " drop " << getDropCount() <<" dropA "<<dropCountA<<" dropB "<<dropCountB<< " enque " << enqueCount<< " deque "<< dequeCount << " overflow " << overflowCount << " vt: "<<currentRound<<"   empty"<<std::endl;

		
		return NULL;
	}
	Ptr<QueueDiscItem> re = NULL;
	//cout<<"earliestlevel"<<earliestLevel<<endl;
	re = FifoDequeue(0);
	//Set the finish time of flow
	string flowlabel = getFlowLabel(re);
        Flow_pl* currFlow = getFlowPtr(flowlabel);
	currFlow->setFinishTime(Simulator::Now().GetSeconds());
	///*pFarbric
	//clear the enque tag for re's flow
	int index = getFlowPtr(getFlowLabel(re))->getFlowNo();
	this->enqueMap[index] = 0;
	//reload the pkt in perflow queue to scheduler
	if (!Per_flow_queues[index]->IsEmpty()) {
            Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(Per_flow_queues[index]->Dequeue());
	    DoEnqueue(Ptr<QueueDiscItem>(item));
        }

	uint32_t remainingsize =  currFlow->getRemaningSize();
	if(remainingsize == 0){
		double FCT = currFlow->getFinishTime() - currFlow->getStartTime();
		RecordFCT("FCT.txt", index,FCT);
	}
	//*/
	
	GearboxPktTag tag;
	Packet* packet = GetPointer(GetPointer(re)->GetPacket());
	packet->PeekPacketTag(tag);
	int departureround = tag.GetDepartureRound();
	Record("DequeuedPktsList.txt", departureround, re);
	stringstream ss;
	ss << "Deque_"<< getFlowPtr(getFlowLabel(re))->getFlowNo()<<".txt";
	string filename = ss.str();
	RecordFlow(getFlowLabel(re), departureround, tag.GetUid(),filename);
	if(departureround%1000 ==0){
		//| (Simulator::Now().GetSeconds()>4.9)
			
			enqueCount = 0;
			dequeCount = 0;
			for(int i = 0; i<DEFAULT_VOLUME; i++){
			
				enqueCount += levels[i].getFifoEnque();
				dequeCount += levels[i].getFifoDeque();
			
			}
			thr <<  Simulator::Now().GetSeconds() <<" uid "<<uid<< " drop " << getDropCount() <<" dropA "<<dropCountA<<" dropB "<<dropCountB<< " enque " << enqueCount<< " deque "<< dequeCount << " overflow " << overflowCount << " vt: "<<currentRound<<"   not empty"<<std::endl;
			
	}

	
	for(int i = 0; i < DEFAULT_VOLUME; i++){
		int currentfifo = levels[i].getEarliestFifo();
		//cout<<currentfifo;
		if( currentfifo == -1){
			currentfifo = 0;
		}
	
	}
	////cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;
	return re;
	
   }
   Ptr<QueueDiscItem> AFQ::FifoDequeue(int earliestLevel) {
	////cout<<endl;
	////cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;
	QueueDiscItem* fifoitem;
	///*SFQ
	int earliestFifo = levels[earliestLevel].getEarliestFifo();
	fifoitem = levels[earliestLevel].fifoDeque(earliestFifo);
	//*/
	/*pFabric
	fifoitem = levels[earliestLevel].fifoDeque(0);
	*/
	if(fifoitem!= 0){
		dequeCount += 1;//sucessfully deque from fifo, deque+1
	}
	GearboxPktTag tag;
	fifoitem->GetPacket()->PeekPacketTag(tag);

	//cout<<"tag.getDepartureRound"<<tag.GetDepartureRound()<<endl;
	/*SFQ
	this->setCurrentRound(tag.GetDepartureRound());
	*/
	Ptr<QueueDiscItem> p = fifoitem;
	setPktCount(pktCount - 1);
	////cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from fifo in level "<< 0 <<endl;
	
	////cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;
	return p;
   }

    void AFQ::setDropCount(int count){
	this->dropCount = count;
    }
    int AFQ::getDropCount(){
	return dropCount;
    }
    Flow_pl* AFQ::insertNewFlowPtr(string fid, int flowNo, int weight, int flowsize, int brustness) { // Peixuan 04212020
	Flowlist.push_back(fid);
        Flow_pl* newFlowPtr = new Flow_pl(fid, flowNo, weight, flowsize, brustness);
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(fid, newFlowPtr));
        return this->flowMap[fid];
    }
    int AFQ::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) { // Peixuan 04212020
	// update flow info
	/*SFQ
        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
        this->flowMap.insert(pair<string, Flow_pl*>(fid, flowPtr));
	*/
	///*pFabric
	uint32_t remainingsize = flowPtr->getRemaningSize();
	if(remainingsize >0){
		flowPtr->setRemaningSize(remainingsize-1);
        	this->flowMap.insert(pair<string, Flow_pl*>(fid, flowPtr));
	}
	//*/
        return 0;
    }
    string AFQ::convertKeyValue(uint64_t fid) { // Peixuan 04212020
        stringstream ss;
        ss << fid;  // Peixuan 04212020
        string key = ss.str();
        return key; //TODO:implement this logic
    }
    Ptr<QueueDiscItem> AFQ::DoRemove(void) {
        //NS_LOG_FUNCTION (this);
        return 0;
    }
    Ptr<const QueueDiscItem> AFQ::DoPeek(void) const {
        //NS_LOG_FUNCTION (this);
        return 0;
    }
    AFQ::~AFQ()
    {
        NS_LOG_FUNCTION(this);
    }
    TypeId
        AFQ::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::AFQ")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<AFQ>()
            ;
        return tid;
    }
    bool AFQ::CheckConfig(void)
    {
        NS_LOG_FUNCTION(this);
        if (GetNQueueDiscClasses() > 0)
        {
            NS_LOG_ERROR("AFQ cannot have classes");
            return false;
        }
        if (GetNPacketFilters() != 0)
        {
            NS_LOG_ERROR("AFQ needs no packet filter");
            return false;
        }
        if (GetNInternalQueues() > 0)
        {
            NS_LOG_ERROR("AFQ cannot have internal queues");
            return false;
        }
        return true;
    }
    void AFQ::InitializeParams(void)
    {
        NS_LOG_FUNCTION(this);
    }
    void AFQ::Record(string fname, int departureRound, Ptr<QueueDiscItem> item){
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

   }

    void AFQ::RecordFlow(string flowlabel, int departureRound, int uid,string filename){
	string path = "GBResult/pktsList/";
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
    void AFQ::Rate(int enqueCount, int dequeCount, int overflowCount, int reloadCount, int migrationCount){
		curTime = Simulator::Now();
		if((curTime.GetSeconds () - prevTime1.GetSeconds ())>1){
			prevTime1 = curTime;
		}
    }
    void AFQ::PacketRecord(string fname, int departureRound, Ptr<QueueDiscItem> item){

		GearboxPktTag tag;
        	item->GetPacket()->PeekPacketTag(tag);

		
		
   }
    void AFQ::tagRecord(int flowiid,int uid, int departureRound, Ptr<QueueDiscItem> item){
		//GearboxPktTag tag;
        	//item->GetPacket()->PeekPacketTag(tag);
		//int dp = tag.GetDepartureRound();
		//int uid = tag.GetUid();
		FILE *fp2;
		
		string path = "GBResult/pktsList/tagRecord";
		//path.append(to_string(flowiid));
		path.append(".txt");
		fp2 = fopen(path.data(), "a+"); //open and write
		fprintf(fp2, "%f", Simulator::Now().GetSeconds());
		fprintf(fp2, "  %s", "vt:");
		fprintf(fp2, "%d", currentRound);
		fprintf(fp2, "  %s", "dp:");
		fprintf(fp2, "%d", departureRound);
		fprintf(fp2, "  %s", "uid:");
		fprintf(fp2, "%d", uid);
		fprintf(fp2, "  %s", "fifoenque:");
		fprintf(fp2, "%d", 0);
		fprintf(fp2, "%s", "  ");
		fprintf(fp2, "%s", "fifodeque:");
		fprintf(fp2, "%d", 0);
		fprintf(fp2, "%s", "  ");
		fprintf(fp2, "%s", "overflow:");
		fprintf(fp2, "%d", 0);
		fprintf(fp2, "%s", "  ");
		fprintf(fp2, "%s", "queueing_delay:");
		fprintf(fp2, "%lf", 0.000000);
		fprintf(fp2, "%s", "  ");
		fprintf(fp2, "%s", "enquetime:");
		fprintf(fp2, "%lf", 0.000000);
		fprintf(fp2, "%s", "  ");
		fprintf(fp2, "%s", "dequetime:");
		fprintf(fp2, "%lf",  0.000000);
		fprintf(fp2, "\n");
		fclose(fp2);
    }

    void AFQ::RecordFCT(string filename, int index,double FCT){
	string path = "GBResult/";
	path.append(filename);

	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "%d\t", index);
	fprintf(fp, "%f", FCT);
	fprintf(fp, "\n");
	fclose(fp);
    }

}
	

