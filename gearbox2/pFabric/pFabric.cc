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
#include <algorithm>
#include "ns3/ipv4-header.h"
#include "ns3/ppp-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ptr.h"
#include "pFabric.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-queue-disc-item.h"


using namespace std;

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("pFabric");
    NS_OBJECT_ENSURE_REGISTERED(pFabric);

    pFabric::pFabric() :pFabric(DEFAULT_VOLUME) {
    }

    pFabric::pFabric(int volume) {
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
	infile.open("scratch/weights.txt");
	int flownumber = 0;
	infile >> flownumber;
        for (int i = 0; i < flownumber; i++){
		int temp = 0;
		infile >> temp;
		weights.push_back(temp);
	}
	infile.close();
	
	ifstream infile1;
	infile1.open("scratch/traffic.txt");
	infile1 >> flownumber;
	for (int j = 0; j < flownumber; j++){
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

    }


    void pFabric::setCurrentRound(int currentRound) {
	if(this->currentRound <= currentRound){
		this->currentRound = currentRound;
	}

	// there is only one level, with granularity of 100 = FIFO_PER_LEVEL * FIFO_PER_LEVEL 
	int level_currentFifo = currentRound / GRANULARITY % FIFO_PER_LEVEL;
	//ifCurrentFifoChange(level_currentFifo);
	levels[0].setCurrentIndex(level_currentFifo);
    }


    void pFabric::setPktCount(int pktCount) {
        this->pktCount = pktCount;
    }


    int pFabric::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	//for adatping pifo, simplyfy the rank computation to 
	int departureRound = currFlow->getRemaningSize();
	return departureRound;
    }


    string pFabric::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	TcpHeader header;
    	GetPointer(item)->GetPacket()->PeekHeader(header);

	stringstream ss;
        ss << ipHeader.GetSource ().Get ()<<header.GetSourcePort()<<header.GetDestinationPort();  // Peixuan 04212020
	//cout<<ipHeader.GetSource ().Get ()<<endl;
        string flowLabel = ss.str();

	return flowLabel;
    }



    Flow_pl* pFabric::getFlowPtr(string flowlabel) {
        string key = convertKeyValue(flowlabel);
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    int weight = DEFAULT_WEIGHT;
	    //int weight = weights.at(flowNo);
	    uint32_t flowSize = flowsize.at(flowNo);
	    flowNo++;
	    //initialize the flow info, no pkt enqueued to scheduler, record the corresponding flowlabel
	    this->enqueMap[flowNo] = 0;
	    //this->flowLabelMap[flowNo] = flowlabel;
	    return this->insertNewFlowPtr(flowlabel, flowNo, weight,flowSize, DEFAULT_BURSTNESS);

        }
        return this->flowMap[key];
    }


    bool pFabric::DoEnqueue(Ptr<QueueDiscItem> item) {
        NS_LOG_FUNCTION(this);

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	int departureRound;
	
	QueueDiscItem* re;
	int index = getFlowPtr(getFlowLabel(item))->getFlowNo();
	
	bool enque = this->enqueMap[index];
	
	//cout<<index<<' '<<enque<<endl;
	if(enque == 0){
		uid = uid + 1;
		maxUid = uid > maxUid ? uid : maxUid;

		string flowlabel = getFlowLabel(item);
		Flow_pl* currFlow = getFlowPtr(flowlabel);
	
	
		// calculate rank value, which is the value of departure round in this case	
		departureRound = RankComputation(GetPointer(item), currFlow);
		
		/*if(departureRound >=60 && departureRound<=150){
			cout << " EN vt:" << currentRound << " dp:" << departureRound << endl;
			levels[0].pifoPrint();
			for (int fifo = 0;fifo<10;fifo++){
				levels[0].fifoPrint(fifo);
			}
		}*/
		stringstream ss;
		ss << "Enque_"<< getFlowPtr(getFlowLabel(item))->getFlowNo()<<".txt";
		string filename = ss.str();
		RecordFlow(flowlabel, departureRound,uid,filename);
		/*//record the difference of the rank and vt to file
		rankDifference[departureRound - currentRound] += 1;
		std::ofstream thr ("GBResult/pktsList/rankVtDifference.dat", std::ios::out | std::ios::app);
		thr << departureRound - currentRound << " ";
		// Gearbox size and drop
		int total = levels[0].getPifoSize();
		for (int l = 0; l < FIFO_PER_LEVEL; l++){
			total += levels[0].getFifoNPackets(l);
		}
		std::ofstream thr2 ("GBResult/pktsList/GB_size.dat", std::ios::out | std::ios::app);
		thr2 << Simulator::Now().GetSeconds() << " " << total << endl;*/
		
		AddTag(0, departureRound, uid, GetPointer(item));
		//Record("EnqueuedPktsList.txt", departureRound, item);
		//cout<<departureRound<<" "<<currentRound<<" "<<FIFO_PER_LEVEL<<" "<<GRANULARITY<<endl;
		if ((departureRound - currentRound) >= FIFO_PER_LEVEL * GRANULARITY) {
			std::ofstream thr9 ("GBResult/pktsList/drop_pkts.dat", std::ios::out | std::ios::app);
			if(Simulator::Now().GetSeconds()>1){
				thr9 << departureRound <<"_flow "<< getFlowPtr(getFlowLabel(item))->getFlowNo()  <<endl;
			}
			else{
				thr9 << departureRound <<"_flow "<< getFlowPtr(getFlowLabel(item))->getFlowNo() <<" "<< uid<<" "<<Simulator::Now().GetSeconds() <<endl;
			}
		
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
			std::ofstream thr8 ("GBResult/pktsList/drop_pkts.dat", std::ios::out | std::ios::app);
			if(Simulator::Now().GetSeconds()>1){
				thr8 << departureRound <<"_flow "<< getFlowPtr(getFlowLabel(item))->getFlowNo()  <<endl;
			}
			else{
				thr8 << departureRound <<"_flow "<< getFlowPtr(getFlowLabel(item))->getFlowNo()<<" "<<uid <<" "<< Simulator::Now().GetSeconds() <<endl;
			}
		    	Drop(item);

			std::ofstream thr4 ("GBResult/pktsList/drop.dat", std::ios::out | std::ios::app);
			thr4 << Simulator::Now().GetSeconds() << " "  << 0 << endl;

			dropCount = dropCount+1;
			this->setDropCount(dropCount);

		    	return false;   // exceeds the maximum burstness

		}	
		
		
	

		//if the flow which this packet belongs to hasn't been enqueued to scheduler, then enque to scheduler
		//else, enque to per flow queue
	
		// enqueue into PIFO first
		re = levels[0].pifoEnque(GetPointer(item));

		// redirect to fifo if not enque successfully into pifo 
		if (re != 0) {
			// re may no be the orginial item
			GearboxPktTag tag0;
			GetPointer(re->GetPacket())->PeekPacketTag(tag0);
			int index = tag0.GetDepartureRound() / GRANULARITY % FIFO_PER_LEVEL;

			re = levels[0].fifoEnque(re, index);

			/*if (tag0.GetDepartureRound()==8976){
			cout<<"pifo fail, Enque to fifo"<<endl;

			levels[0].pifoPrint();

				for (int fifo = 0;fifo<30;fifo++){
					levels[0].fifoPrint(fifo);
				}

			}*/
			// during reloading process, update pifomaxtag if the returned pkt's rank is smaller than the current pifomaxtag
			if (levels[0].getReloadHold() == 1 && tag0.GetDepartureRound() < levels[0].getPifoMaxValue()){
				levels[0].setPifoMaxValue(tag0.GetDepartureRound());
			}
			/*if (re != 0){
			GearboxPktTag tag2;
			GetPointer(re->GetPacket())->PeekPacketTag(tag2);
			cout<<"re "<< tag2.GetDepartureRound() <<endl;
			}*/
			// fifo is full
			if (re != 0){
			
			

				GearboxPktTag tag1;
				GetPointer(re->GetPacket())->PeekPacketTag(tag1);
				index = tag1.GetDepartureRound() / GRANULARITY % FIFO_PER_LEVEL;
				/*if (tag1.GetDepartureRound()==8976&&getFlowPtr(getFlowLabel(re))->getFlowNo()==4){
				cout<<"fifo fail, drop"<<endl;
				cout << " EN vt:" << currentRound << " dp:" << departureRound <<"flow num:"<< getFlowPtr(getFlowLabel(re))->getFlowNo()<<"index"<<index<<endl;

				levels[0].pifoPrint();

					for (int fifo = 0;fifo<30;fifo++){
						levels[0].fifoPrint(fifo);
					}

				}*/
	
			// during reloading process, update pifomaxtag if the returned pkt's rank is smaller than the current pifomaxtag
			if (levels[0].getReloadHold() == 1 && tag0.GetDepartureRound() < levels[0].getPifoMaxValue()){
				levels[0].setPifoMaxValue(tag0.GetDepartureRound());
			}


				std::ofstream thr7 ("GBResult/pktsList/drop_pkts.dat", std::ios::out | std::ios::app);
				if(Simulator::Now().GetSeconds()>1){
					thr7 << tag1.GetDepartureRound() <<"_flow "<< getFlowPtr(getFlowLabel(re))->getFlowNo() <<endl;
				}
				else{
					thr7 << tag1.GetDepartureRound() <<"_flow "<< getFlowPtr(getFlowLabel(re))->getFlowNo()<<" "<<uid <<" "<< Simulator::Now().GetSeconds() <<endl;
				}
				Drop(re);//fifo is full


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
				this->enqueMap[index] = 1;
				this->updateFlowPtr(departureRound, flowlabel, currFlow); 
				return true;
			}
		}
		else {
			/*cout<<"!"<<endl;
			levels[0].pifoPrint();

					for (int fifo = 0;fifo<30;fifo++){
						levels[0].fifoPrint(fifo);
					}
			setPktCount(pktCount + 1);*/
			this->enqueMap[index] = 1;
			this->updateFlowPtr(departureRound, flowlabel, currFlow); 
		    	return true;
		}
	}
	else{
		//enqueue into per flow queue
		if ((Per_flow_queues[index]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
			Per_flow_queues[index]->Enqueue(Ptr<QueueDiscItem>(item));
		}
		else{
			Drop(item);
			cout<<"drop per flow queue: "<<index<<endl;
		}
		
		return true;
	}
    }


    void pFabric::AddTag(bool level, int departureRound, int uid, QueueDiscItem* item){
	GearboxPktTag tag;
	Packet* packet = GetPointer(item->GetPacket());
	packet->PeekPacketTag(tag);
	float time = Simulator::Now().GetSeconds();

	packet->AddPacketTag(GearboxPktTag(departureRound, time, uid));
    }



    Ptr<QueueDiscItem> pFabric::DoDequeue() {
	
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
	//deque the first pkt in pifo 
	re = PifoDequeue();
	//clear the enque tag for re's flow
	int index = getFlowPtr(getFlowLabel(re))->getFlowNo();
	this->enqueMap[index] = 0;
	//reload the pkt in perflow queue to scheduler
	if (!Per_flow_queues[index]->IsEmpty()) {
            Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(Per_flow_queues[index]->Dequeue());
	    DoEnqueue(Ptr<QueueDiscItem>(item));
        }
        
	this->Reload();

	//Set the finish time of flow
	string flowlabel = getFlowLabel(re);
        Flow_pl* currFlow = getFlowPtr(flowlabel);
	currFlow->setFinishTime(Simulator::Now().GetSeconds());

	uint32_t remainingsize =  currFlow->getRemaningSize();
	if(remainingsize == 0){
		double FCT = currFlow->getFinishTime() - currFlow->getStartTime();
		RecordFCT("FCT.txt", index,FCT);
	}

	GearboxPktTag tag;
	Packet* packet = GetPointer(GetPointer(re)->GetPacket());
	packet->PeekPacketTag(tag);
	//tag.SetDequeTime(Simulator::Now().GetSeconds());
	//packet->ReplacePacketTag(tag);

	int departureround = tag.GetDepartureRound();

	Record("DequeuedPktsList.txt", departureround, re);
	stringstream ss;
	ss << "Deque_"<< getFlowPtr(getFlowLabel(re))->getFlowNo()<<".txt";
	string filename = ss.str();
	RecordFlow(getFlowLabel(re), departureround,tag.GetUid(), filename );

	// statistic per 1000 packets dequeued
	if (tag.GetUid() % 100 == 0){
		int total = levels[0].getFifoEnque() + levels[0].getFifoDeque() + levels[0].getPifoEnque() + levels[0].getPifoDeque();
		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out);
		thr <<  Simulator::Now().GetSeconds() << " TOTAL:" << total << " fifoenque:" << levels[0].getFifoEnque() << " fifodeque:" << levels[0].getFifoDeque()
			<< " pifoenque:" << levels[0].getPifoEnque() << " pifodeque:" << levels[0].getPifoDeque()
			<< " reload:"<< levels[0].getReload() << " reloadIni:" << levels[0].getReloadIni() 
			<< " uid:" << maxUid << " vt:" << currentRound 
                        << " drop:" << getDropCount() << " = dropA:" << dropCountA << " + dropB:" << dropCountB << " - dropC:" << dropCountC
			<< " flowNo:" << flowNo << " non-empty" << std::endl;
	}

	/*for(int i = 0; i < DEFAULT_VOLUME; i++){
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
	}*/
	
	return re;
   }



   Ptr<QueueDiscItem> pFabric::FifoDequeue(int earliestLevel) {
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
	//this->setCurrentRound(tag.GetDepartureRound());
	Ptr<QueueDiscItem> p = fifoitem;
	setPktCount(pktCount - 1);

	return p;
   }



   Ptr<QueueDiscItem> pFabric::PifoDequeue() {

   	QueueDiscItem* pifoitem = levels[0].pifoDeque();
	GearboxPktTag tag;
	pifoitem->GetPacket()->PeekPacketTag(tag);
	//this->setCurrentRound(tag.GetDepartureRound());

	Ptr<QueueDiscItem> p = pifoitem;
	setPktCount(pktCount - 1);
	return p;

   }


   void pFabric::Reload(){

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



    void pFabric::setDropCount(int count){
	this->dropCount = count;
    }


    int pFabric::getDropCount(){
	return dropCount;
    }


    Flow_pl* pFabric::insertNewFlowPtr(string fid, int fno, int weight, int flowsize,int burstness) {
	Flowlist.push_back(fid);

        string key = convertKeyValue(fid); 
        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight,flowsize, burstness);
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));

        return this->flowMap[key];
    }



    int pFabric::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) {
        string key = convertKeyValue(fid); 
	// update flow remainingsize only when it's positive
	uint32_t remainingsize = flowPtr->getRemaningSize();
	if(remainingsize >0){
		flowPtr->setRemaningSize(remainingsize-1);
        	this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));
	}
	
        return 0;
    }


    string pFabric::convertKeyValue(string fid) { 
        stringstream ss;
        ss << fid; 
        string key = ss.str();

        return key;
    }



    Ptr<QueueDiscItem> pFabric::DoRemove(void) {
        return 0;
    }


    Ptr<const QueueDiscItem> pFabric::DoPeek(void) const {
        return 0;
    }



    pFabric::~pFabric()
    {
        NS_LOG_FUNCTION(this);
    }



    TypeId pFabric::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::pFabric")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<pFabric>();

        return tid;
    }



    bool pFabric::CheckConfig(void)
    {
        NS_LOG_FUNCTION(this);
        if (GetNQueueDiscClasses() > 0){
            NS_LOG_ERROR("pFabric cannot have classes");
            return false;
        }

        if (GetNPacketFilters() != 0){
            NS_LOG_ERROR("pFabric needs no packet filter");
            return false;
        }

        if (GetNInternalQueues() > 0) {
            NS_LOG_ERROR("pFabric cannot have internal queues");
            return false;
        }

        return true;
    }



    void pFabric::InitializeParams(void)
    {
        NS_LOG_FUNCTION(this);
    }


    void pFabric::Record(string fname, int departureRound, Ptr<QueueDiscItem> item){
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



    void pFabric::RecordFlow(string flowlabel, int departureRound, int uid,string filename){
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
    void pFabric::RecordFCT(string filename, int index,double FCT){
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







	
