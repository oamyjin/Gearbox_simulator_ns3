#include "ns3/Level_afq_pFabric.h"
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
#include "AFQ_pFabric.h"
#include "ns3/queue.h"
#include "ns3/queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-queue-disc-item.h"


using namespace std;

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("AFQ_pFabric");
    NS_OBJECT_ENSURE_REGISTERED(AFQ_pFabric);

    AFQ_pFabric::AFQ_pFabric() :AFQ_pFabric(DEFAULT_VOLUME) {
    }

    AFQ_pFabric::AFQ_pFabric(int volume) {
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

	int no = 0;


	ifstream infile1;
	infile1.open("scratch/traffic_pFabric.txt");
	infile1 >> no;
	for (int j = 0; j < no; j++){
		int maxPacketCount;
		uint32_t src, dst, dport;
		double start_time;
		//double start_time,stop_time;
		infile1 >> src >> dst >> dport >> maxPacketCount >> start_time;
		//infile1 >> src >> dst >> dport >> maxPacketCount >> start_time>>stop_time;
		flowsize.push_back(maxPacketCount);
	}
	infile1.close();

	/*
	ifstream infile2;
	string pkts_size;
	int line = 0;
	infile2.open("scratch/packets.txt");
	while (getline (infile2, pkts_size)){
			
			//cout << pkts_size << endl;
			char input[100000];
			//cout<<(int)pkts_size.length()<<endl;
			for(int i = 0; i < (int)pkts_size.length(); i++){
				input[i] = pkts_size[i];
			}
			input[pkts_size.length()] = '\0';
			const char *d = " ";
			char *p;
			//cout<<input<<endl;
			p = strtok(input,d);
			while(p)
			{

			        packetList[line].push_back(atoi(p));
				p=strtok(NULL,d);
				//int packetsize = packetList[line].back();
				//cout<<packetsize<<endl;
			}
			
			line++;
	}*/

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


    void AFQ_pFabric::setCurrentRound(int cr) {
	if(this->currentRound <= cr){
		this->currentRound = cr;
	}

	// there is only one level, with granularity of 0 = FIFO_PER_LEVEL * FIFO_PER_LEVEL 
	int level_currentFifo = currentRound / GRANULARITY % FIFO_PER_LEVEL; //(FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
	//ifCurrentFifoChange(level_currentFifo);
	levels[0].setCurrentIndex(level_currentFifo);
    }


    void AFQ_pFabric::setPktCount(int pktCount) {
        this->pktCount = pktCount;
    }


    int AFQ_pFabric::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){
	// calculate the rank(departureRound) value
	int departureRound = currFlow->getRemaningSize();
	return departureRound;
    }


    string AFQ_pFabric::getFlowLabel(Ptr<QueueDiscItem> item){
	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	TcpHeader header;
    	GetPointer(item)->GetPacket()->PeekHeader(header);

	stringstream ss;
        ss << ipHeader.GetSource ().Get ()<<header.GetSourcePort()<<header.GetDestinationPort();  // Peixuan 04212020
        string flowLabel = ss.str();
	return flowLabel;
    }



    Flow_pl* AFQ_pFabric::getFlowPtr(string flowlabel) {
        string key = convertKeyValue(flowlabel);
	// insert new flows with assigned weight
        if (flowMap.find(key) == flowMap.end()) {
	    int weight = DEFAULT_WEIGHT;
	    //int weight = weights.at(flowNo);
	    int fs = flowsize.at(flowNo);
	    //vector<int> packet_List = packetList[flowNo];
	    flowNo++;
	    this->enqueMap[flowNo] = 0;
	    return this->insertNewFlowPtr(flowlabel,flowNo, weight, fs, {}, DEFAULT_BURSTNESS);

        }
        return this->flowMap[key];
    }


    bool AFQ_pFabric::DoEnqueue(Ptr<QueueDiscItem> item) {
        NS_LOG_FUNCTION(this);
	// debug
	/*if(currentRound >= 69109 && currentRound <= 4000){
		cout << endl << "vt:" << currentRound << endl;
		levels[0].fifoPktPrint(9);
	}*/

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);
	const Ipv4Header ipHeader = ipItem->GetHeader();
	int departureRound;
	int index1 = getFlowPtr(getFlowLabel(item))->getFlowNo();
	bool enque = this->enqueMap[index1];
	if(enque == 0){
		uid = uid + 1;
		maxUid = uid > maxUid ? uid : maxUid;

		string flowlabel = getFlowLabel(item);
		Flow_pl* currFlow = getFlowPtr(flowlabel);
		int flowNo = currFlow->getFlowNo();

		// calculate rank value, which is the value of departure round in this case	
		departureRound = RankComputation(GetPointer(item), currFlow);
		// record the difference of the rank and vt to file
		//rankDifference[departureRound - currentRound] += 1;

		/*std::ofstream thr ("GBResult_pFabric/pktsList/rankVtDifference.dat", std::ios::out | std::ios::app);
		thr << departureRound - currentRound << " ";*/

		// Gearbox size and drop
		int total = levels[0].getPifoSize();
		for (int l = 0; l < FIFO_PER_LEVEL; l++){
			total += levels[0].getFifoNPackets(l);
		}
		std::ofstream thr2 ("GBResult_pFabric/pktsList/GB_size.dat", std::ios::out | std::ios::app);
		thr2 << Simulator::Now().GetSeconds() << " " << total << endl;

		AddTag(flowNo, departureRound, uid, GetPointer(item));
		//cout<<"addtag"<<departureRound<<endl;
		Record("EnqueuedPktsList.txt", departureRound, item);	
		
		if (departureRound > ((currentRound / GRANULARITY) * GRANULARITY + (GRANULARITY * FIFO_PER_LEVEL) - 1)) {// FIFO_PER_LEVEL * GRANULARITY) {
		    	Drop(item);

			stringstream path1;
			path1 << "GBResult_pFabric/pktsList/dropRecord" << flowNo << ".dat"; //plotResult
			std::ofstream thr3 (path1.str(), std::ios::out | std::ios::app);
			thr3 << Simulator::Now().GetSeconds() << " "  << 0 << endl;

			dropCount = dropCount+1;
			dropCountA = dropCountA+1;
			this->setDropCount(dropCount);

		    	return false;   //exceeds the maximum round
		}

		int curBurstness = currFlow->getBurstness();
		if ((departureRound - currentRound) >= curBurstness) {
		    	Drop(item);

			stringstream path2;
			path2 << "GBResult_pFabric/pktsList/dropRecord" << flowNo << ".dat"; //plotResult
			std::ofstream thr4 (path2.str(), std::ios::out | std::ios::app);
			thr4 << Simulator::Now().GetSeconds() << " "  << 0 << endl;

			dropCount = dropCount+1;
			dropCountA = dropCountA+1;
			this->setDropCount(dropCount);
			
		    	return false;   // exceeds the maximum burstness

		}		
		// this->updateFlowPtr(departureRound, flowlabel, currFlow); 
	
		// debug
		if(currentRound >= 69109 && currentRound <= 6800){
			//cout << endl << "vt:" << currentRound << " " << Simulator::Now().GetSeconds() << endl << " EQ dp: " << departureRound << " uid:" << uid << " fid:" << flowNo << endl;
			levels[0].pifoPrint();
			for (int i = 0; i < FIFO_PER_LEVEL; i++){
				levels[0].fifoPrint(i);
			}
		}


			int index = departureRound / GRANULARITY % FIFO_PER_LEVEL; // (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;
			QueueDiscItem* re = levels[0].fifoEnque(GetPointer(item), index);

			// fifo is full
			if (re != 0){
				Drop(re);

				/*if (tag1.GetFifodeque() + tag1.GetPifodeque() != 0){
					std::ofstream thr5 ("GBResult_pFabric/pktsList/drop.dat", std::ios::out | std::ios::app);
					thr5 << Simulator::Now().GetSeconds() << " " << tag1.GetFifodeque() << " " << tag1.GetPifodeque() << endl;
					dropCountC += 1;
				}*/
				stringstream path3;
				path3 << "GBResult_pFabric/pktsList/dropRecord" << flowNo << ".dat"; //plotResult
				std::ofstream thr7 (path3.str(), std::ios::out | std::ios::app);
				thr7 << Simulator::Now().GetSeconds() << " "  << 0 << endl;
				std::ofstream thr6 ("GBResult_pFabric/pktsList/fifo_drop.dat", std::ios::out | std::ios::app);
				thr6 << (index - levels[0].getEarliestFifo() + FIFO_PER_LEVEL) % FIFO_PER_LEVEL << " ";

				dropCount = dropCount+1;
				dropCountB = dropCountB+1;
				this->setDropCount(dropCount);
				// debug
				if(currentRound >= 69109 && currentRound <= 6800){
					cout << "drop" << endl;
				}
				return false;
			}
			else {
				// debug
				if(currentRound >= 69109 && currentRound <= 6800){
					levels[0].pifoPrint();
					for (int i = 0; i < FIFO_PER_LEVEL; i++){
						levels[0].fifoPrint(i);
					}
				}

				this->enqueMap[index1] = 1;
				this->updateFlowPtr(departureRound, flowlabel, currFlow); // enqueued succefuly into fifo
				levels[0].addSchePkt(uid, departureRound); // record into the map which records all pkts who are currently in the system
				//record each flow
				stringstream ss;
				ss << "Enque_"<< index1<<".txt";
				string filename = ss.str();
				RecordFlow(flowlabel, departureRound, uid, filename );
				return true;
			}

	
	}
	else{
		if ((Per_flow_queues[index1]->GetNPackets() < DEFAULT_FIFO_N_SIZE)){
			Per_flow_queues[index1]->Enqueue(Ptr<QueueDiscItem>(item));
			return true;
		}
		else{
			Drop(item);
			cout<<"drop per flow queue: "<<index1<<endl;
			drop_perQ += 1;
			return false;
		}
		
		
	}

	
    }


    void AFQ_pFabric::AddTag(int flowid, int departureRound, int uid, QueueDiscItem* item){
	GearboxPktTag tag;
	Packet* packet = GetPointer(item->GetPacket());
	packet->PeekPacketTag(tag);
	//float time = Simulator::Now().GetSeconds();

	packet->AddPacketTag(GearboxPktTag(flowid, uid, departureRound));
	//packet->AddPacketTag(GearboxPktTag(departureRound, time, uid));
    }



    Ptr<QueueDiscItem> AFQ_pFabric::DoDequeue() {
	// no more pkts in GB
	if(levels[0].getPifoSize() == 0 && levels[0].getFifoTotalNPackets() == 0){
		Record("CountStat.dat", 0, NULL);
		return NULL;
	}

	// debug
	if(currentRound >= 69109 && currentRound <= 6800){
		cout << endl << "vt:" << currentRound << " " << Simulator::Now().GetSeconds() << endl << " DQ" <<  " rl:" << levels[0].getReloadHold() << endl;
		levels[0].pifoPrint();
		for (int i = 0; i < FIFO_PER_LEVEL; i++){
			levels[0].fifoPrint(i);
		}
	}

	Ptr<QueueDiscItem> re = NULL;
	
	re = FifoDequeue(0);
	this->Reload();

	GearboxPktTag tag;
	Packet* packet = GetPointer(GetPointer(re)->GetPacket());
	packet->PeekPacketTag(tag);
	int departureround = tag.GetDepartureRound();
	int uid = tag.GetUid();
	int index = getFlowPtr(getFlowLabel(re))->getFlowNo();

	// update the map which records all pkts currently in the scheduler
	levels[0].removeSchePkt(uid);
	int inversion = levels[0].cal_inversion_mag(departureround);
	if (inversion > 0){
		inv_count += 1;
		inv_mag += inversion;
	}
	std::ofstream thr2 ("GBResult_pFabric/inversion_record.dat", std::ios::out);
	thr2 << "count: " << inv_count << "  magnitude: " << inv_mag << endl;
	//clear the enque tag for re's flow
	this->enqueMap[index] = 0;
	//reload the pkt in perflow queue to scheduler
	if (!Per_flow_queues[index]->IsEmpty()) {
            Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(Per_flow_queues[index]->Dequeue());
	    DoEnqueue(Ptr<QueueDiscItem>(item));
        }	

	

	//Set the finish time of flow
	string flowlabel = getFlowLabel(re);
        Flow_pl* currFlow = getFlowPtr(flowlabel);
	currFlow->setFinishTime(Simulator::Now().GetSeconds());

	int remainingsize =  currFlow->getRemaningSize();
	if(remainingsize == 0){
		double FCT = currFlow->getFinishTime() - currFlow->getStartTime();
		RecordFCT("FCT.txt", index,FCT);
	}


	Record("DequeuedPktsList.txt", departureround, re);
	Record("CountStat.dat", departureround, re);
	//record each flow
	stringstream ss;
	ss << "Deque_"<< getFlowPtr(getFlowLabel(re))->getFlowNo()<<".txt";
	string filename = ss.str();
	RecordFlow(getFlowLabel(re), departureround,tag.GetUid(), filename );


	/*std::ofstream thr ("GBResult_pFabric/pktsList/relativefifo_size.dat", std::ios::out | std::ios::app);
	int currentfifo = levels[0].getEarliestFifo();
	if( currentfifo == -1){
		currentfifo = 0;
	}
	for (int j = 0; j < FIFO_PER_LEVEL; j++){
		
		thr << levels[0].getFifoNPackets((j+currentfifo)%FIFO_PER_LEVEL) << " ";
	}
	thr << "\n";
	for(int i = 0; i < DEFAULT_VOLUME; i++){
		int currentfifo = levels[i].getEarliestFifo();
		if( currentfifo == -1){
			currentfifo = 0;
		}
		for (int j = 0; j < FIFO_PER_LEVEL; j++){
			int fifosize = levels[i].getFifoNPackets((j+currentfifo)%FIFO_PER_LEVEL);
				FILE *fp;
				string path = "GBResult_pFabric/pktsList/Level_";
				path.append(to_string(i));
				path.append("_fifo_");
				path.append(to_string(j));
				path.append("_Record.txt");
				fp = fopen(path.data(), "a+"); //open and write
				fprintf(fp, "%d ", fifosize);
				fclose(fp);
		}
	}*/

	// debug
	if(currentRound >= 69109 && currentRound <= 6800){
		cout << "=========== DQ dp:" << departureround << " fid:" << currFlow->getFlowNo() << endl;
		levels[0].pifoPrint();
		for (int i = 0; i < FIFO_PER_LEVEL; i++){
			levels[0].fifoPrint(i);
		}
	}

	
	/*
	if(inversion>0){
			cout<<"dp"<<departureround<<endl;
		levels[0].pifoPrint();
				for (int i = 0; i < FIFO_PER_LEVEL; i++){
					levels[0].fifoPrint(i);
				}
			}
	*/
	return re;
   }



   Ptr<QueueDiscItem> AFQ_pFabric::FifoDequeue(int earliestLevel) {
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



   Ptr<QueueDiscItem> AFQ_pFabric::PifoDequeue() {

   	QueueDiscItem* pifoitem = levels[0].pifoDeque();
	GearboxPktTag tag;
	pifoitem->GetPacket()->PeekPacketTag(tag);
	//this->setCurrentRound(tag.GetDepartureRound());
	//cout<<"deque"<<tag.GetDepartureRound()<<endl;

	Ptr<QueueDiscItem> p = pifoitem;
	setPktCount(pktCount - 1);
	return p;

   }


   void AFQ_pFabric::Reload(){
	// debug
	if (currentRound >= 69109 && currentRound <= 6800){	
		cout << "reload" << endl;
	}
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



    void AFQ_pFabric::setDropCount(int count){
	this->dropCount = count;
    }


    int AFQ_pFabric::getDropCount(){
	return dropCount;
    }


    Flow_pl* AFQ_pFabric::insertNewFlowPtr(string fid, int fno, int weight, int flowsize, vector<int> packet_List,int burstness) {
	Flowlist.push_back(fid);

        string key = convertKeyValue(fid); 
        Flow_pl* newFlowPtr = new Flow_pl(fid, fno, weight, burstness,flowsize);
	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));

        return this->flowMap[key];
    }



    int AFQ_pFabric::updateFlowPtr(int departureRound, string fid, Flow_pl* flowPtr) {
        string key = convertKeyValue(fid); 
	// update flow info
        int remainingsize = flowPtr->getRemaningSize();
	int remaining = 0 > (remainingsize-1) ? 0 : remainingsize-1;//
	flowPtr->setRemaningSize(remaining);
	this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));

        return 0;
    }


    string AFQ_pFabric::convertKeyValue(string fid) { 
        stringstream ss;
        ss << fid; 
        string key = ss.str();

        return key;
    }



    Ptr<QueueDiscItem> AFQ_pFabric::DoRemove(void) {
        return 0;
    }


    Ptr<const QueueDiscItem> AFQ_pFabric::DoPeek(void) const {
        return 0;
    }



    AFQ_pFabric::~AFQ_pFabric()
    {
        NS_LOG_FUNCTION(this);
    }



    TypeId AFQ_pFabric::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::AFQ_pFabric")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<AFQ_pFabric>();

        return tid;
    }



    bool AFQ_pFabric::CheckConfig(void)
    {
        NS_LOG_FUNCTION(this);
        if (GetNQueueDiscClasses() > 0){
            NS_LOG_ERROR("AFQ_pFabric cannot have classes");
            return false;
        }

        if (GetNPacketFilters() != 0){
            NS_LOG_ERROR("AFQ_pFabric needs no packet filter");
            return false;
        }

        if (GetNInternalQueues() > 0) {
            NS_LOG_ERROR("AFQ_pFabric cannot have internal queues");
            return false;
        }

        return true;
    }



    void AFQ_pFabric::InitializeParams(void)
    {
        NS_LOG_FUNCTION(this);
    }


    void AFQ_pFabric::Record(string fname, int departureRound, Ptr<QueueDiscItem> item){
	if (fname.compare("CountStat.dat") == 0){
		int total = levels[0].getFifoEnque() + levels[0].getFifoDeque() + levels[0].getPifoEnque() + levels[0].getPifoDeque();
		std::ofstream thr ("GBResult_pFabric/CountStat.dat", std::ios::out);
		thr <<  Simulator::Now().GetSeconds() << " TOTAL:" << total << " fifoenque:" << levels[0].getFifoEnque() << " fifodeque:" << levels[0].getFifoDeque()
			<< " pifoenque:" << levels[0].getPifoEnque() << " pifodeque:" << levels[0].getPifoDeque()
			<< " reload:"<< levels[0].getReload() << " reloadIni:" << levels[0].getReloadIni() 
			<< " uid:" << maxUid << " vt:" << currentRound 
                        << " drop:" << getDropCount() << " = dropA:" << dropCountA << " + dropB:" << dropCountB << " - dropC:" << dropCountC <<", drop_perQ" << drop_perQ
			<< " flowNo:" << flowNo << " empty" << std::endl;
		return;
	}

	GearboxPktTag tag;
        item->GetPacket()->PeekPacketTag(tag);
	
	int dp = tag.GetDepartureRound();
	
	string path = "GBResult_pFabric/pktsList/";
	path.append(fname);

	/*Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);  
	const Ipv4Header ipHeader = ipItem->GetHeader();  TcpHeader header;      
	GetPointer(item)->GetPacket()->PeekHeader(header);  
	string flowlabel = getFlowLabel(item);  
	Flow_pl* currFlow = getFlowPtr(flowlabel); */


	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write 
	//fprintf(fp, "%s", "(");  
	fprintf(fp, "%d", dp);  
	/*fprintf(fp, "%s", ", ");  
	fprintf(fp, "%d", currFlow->getFlowNo());
	fprintf(fp, "%s", ", ");  
	fprintf(fp, "%d", GetPointer(item)->GetPacket()->GetSize());
	fprintf(fp, "%s", ",");
	fprintf(fp, "%f", Simulator::Now().GetSeconds());
	fprintf(fp, "%s", ")"); */ 	
	fprintf(fp, "\t");
	fclose(fp);

	FILE *fpl0;
	fpl0 = fopen("GBResult_pFabric/pktsList/L0Fifo", "a+"); //open and write
	fprintf(fpl0, "%f", Simulator::Now().GetSeconds());
	fprintf(fpl0, "\t%d", levels[0].getFifoMaxNPackets());
	fprintf(fpl0, "\n");
	fclose(fpl0);

		
	FILE *fpl1;
	fpl1 = fopen("GBResult_pFabric/pktsList/L0Pifo", "a+"); //open and write
	fprintf(fpl1, "%f", Simulator::Now().GetSeconds());
	fprintf(fpl1, "\t%d", levels[0].getPifoSize());
	fprintf(fpl1, "\n");
	fclose(fpl1);
   }


    void AFQ_pFabric::RecordFlow(string flowlabel, int departureRound, int uid,string filename){
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

    void AFQ_pFabric::RecordFCT(string filename, int index,double FCT){
	string path = "GBResult_pFabric/";
	path.append(filename);

	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	fprintf(fp, "%d\t", index);
	fprintf(fp, "%f", FCT);
	fprintf(fp, "\n");
	fclose(fp);
    }

}







	

