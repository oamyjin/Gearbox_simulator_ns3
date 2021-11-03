
#include "ns3/Level_flex.h"

#include "ns3/Flow_pl.h"

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

	    //cout << i << " pifopeek:" << departureRound << endl;

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

		//cout << "earliest fifopeek_depRound:" << departureRound << " pifopeek_tagmin:" << tag_min << endl;

		if (departureRound <= tag_min) { //deque level0 first?

		    //cout << "departureRound is "<< departureRound<<endl;

		    tag_min = departureRound;

		    level_min = 0;

		    //cout << "level_min:" << level_min << " fifopeek_depRound:" << departureRound << " index:" << tag.GetIndex() << endl;

		    return 0;

		}

	}

	//cout << "level_min:" << level_min << " tag_min:" << tag_min << endl;

	return level_min;

    }



    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {

        this->currentRound = currentRound;

	int level0_currentFifo = currentRound % FIFO_PER_LEVEL;

	levels[0].setCurrentIndex(level0_currentFifo);

	int level1_currentFifo = currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL;

	levels[1].setCurrentIndex(level1_currentFifo);

	int level2_currentFifo = currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL;

	levels[2].setCurrentIndex(level2_currentFifo);



	cout<<endl;

	cout << "Level1_currentFifo:" << level1_currentFifo << " isCurrentFifoEmpty:" << levels[1].isCurrentFifoEmpty() << " fSize:" << levels[1].getFifoNPackets(level1_currentFifo) << " " << levels[1].getCurrentFifoNPackets() << endl;			

	cout<<endl;

	cout << "Level2_currentFifo:" << level2_currentFifo << " isCurrentFifoEmpty():" << levels[2].isCurrentFifoEmpty() << " fifoSize:" << levels[2].getFifoNPackets(level2_currentFifo)  << " " << levels[2].getCurrentFifoNPackets() << endl;

    }





    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {

        this->pktCount = pktCount;

    }







    int Gearbox_pl_fid_flex::RankComputation(QueueDiscItem* item, Flow_pl* currFlow){

	// calculate the rank(departureRound) value

        //departureRound = cal_theory_departure_round(currFlow, item->GetPacket()->GetSize());

	int curLastFinishRound = currFlow->getLastFinishRound();

        int departureRound = max(currentRound, curLastFinishRound);



	return departureRound;

    }



    uint64_t Gearbox_pl_fid_flex::getFlowLabel(Ptr<QueueDiscItem> item){

	Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);

	const Ipv4Header ipHeader = ipItem->GetHeader();

	cout<<"source:"<<ipHeader.GetSource();

	cout<<" destination:"<<ipHeader.GetDestination();

	uint64_t flowLabel = uint64_t(ipHeader.GetSource ().Get ()) << 32 | uint64_t (ipHeader.GetDestination ().Get ());

	

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

            cout << "The pkt belongs to a new flow." << endl;

	    return this->insertNewFlowPtr(flowlabel, weight, DEFAULT_BRUSTNESS);

        }



        return this->flowMap[key];

    }







    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {

        NS_LOG_FUNCTION(this);

	GearboxPktTag tag;

	Packet* packet = GetPointer(GetPointer(item)->GetPacket());

	bool hasTag = packet->PeekPacketTag(tag);

	

	cout<<endl;

	cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

	cout << "CurrentRound:" << currentRound<< endl;

	cout <<"Before Enque the pifo state is: "<< endl;

	cout << "L1:";

	levels[1].pifoPrint(); 

	cout << "L2:";

	levels[2].pifoPrint();    	

   	

        Ptr<const Ipv4QueueDiscItem> ipItem = DynamicCast<const Ipv4QueueDiscItem>(item);

	const Ipv4Header ipHeader = ipItem->GetHeader();

	cout<<"The address is: "<<endl;

	cout<<"source: "<<ipHeader.GetSource()<<" destination: "<<ipHeader.GetDestination()<<endl;

      

	int departureRound;

	//if the packet is new, hasTag equals to 0, calculate departure round

	//else the packet is migrated, extract the departureRound

        

	bool result;

	if(hasTag == 0){

		cout<<"The pkt is new."<<endl;

		// get the enqueued pkt's flow

	        uint64_t flowlabel = getFlowLabel(item);

           	Flow_pl* currFlow = getFlowPtr(flowlabel);



		// calculate rank value, which is the value of departure round in this case	

	   	departureRound = RankComputation(GetPointer(item), currFlow);



        	if ((departureRound - currentRound ) >= FIFO_PER_LEVEL * FIFO_PER_LEVEL * FIFO_PER_LEVEL) {

	    	cout << " DROP!!! TOO LARGE DEPARTURE_ROUND!  departureRound:"  << departureRound << " currentRound:" << currentRound << endl;

            	Drop(item);

		dropCount = dropCount+1;

		this->setDropCount(dropCount);

            	return false;   // 07072019 Peixuan: exceeds the maximum round

        	}

        	int curBrustness = currFlow->getBrustness();

        	if ((departureRound - currentRound) >= curBrustness) {

	    	cout << " DROP!!! TOO BURST!  departureRound:"  << departureRound << " currentRound:" << currentRound << " curBrustness:" << curBrustness << endl;

            	Drop(item);

		dropCount = dropCount+1;

		this->setDropCount(dropCount);

            	return false;   // 07102019 Peixuan: exceeds the maximum brustness

        	}		



		

		//level 2

		if ((departureRound - currentRound) >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL)){

            		this->updateFlowPtr(departureRound, flowlabel, currFlow);  // Peixuan 04212020 fid

			// Add Pkt Tag

			double time = Simulator::Now().GetSeconds();

			float time2 = Simulator::Now().GetSeconds();

			cout<< "time"<<time << " "<<time2 <<endl;

	        	packet->AddPacketTag(GearboxPktTag(departureRound, departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL, time2));

			packet->PeekPacketTag(tag);

			cout<<"The storage destination is:"<<endl;			

			cout << "departureRound:" << departureRound << " insertLevel:" << 2 << " index:" << departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL<<"time"<<tag.GetEnqueTime()<<endl;

			result = PifoEnqueue(2, GetPointer(item));

		}

		//level 1

		else if ((departureRound - currentRound) >= FIFO_PER_LEVEL){

            		this->updateFlowPtr(departureRound, flowlabel, currFlow);  // Peixuan 04212020 fid

			// Add Pkt Tag

			double time = Simulator::Now().GetSeconds();

			float time2 = Simulator::Now().GetSeconds();

			cout<< "time"<<time << " "<<time2 <<endl;

	        	packet->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL, time2));

			packet->PeekPacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

	    		cout << "departureRound:" << departureRound << " insertLevel:" << 1 << " index:" << departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL<<"time"<<tag.GetEnqueTime()<<endl;

			result = PifoEnqueue(1, GetPointer(item));

		}

		//level 0

		else{

            		this->updateFlowPtr(departureRound, flowlabel, currFlow);

			double time = Simulator::Now().GetSeconds();

			float time2 = Simulator::Now().GetSeconds();

			cout<< "time"<<time << " "<<time2 <<endl;

			packet->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL, time2));

			packet->PeekPacketTag(tag);

			cout<<"The destination to be stored is:"<<endl;

	        	cout << "departureRound:" << departureRound << " insertLevel:" << 0 << " index:" << departureRound % FIFO_PER_LEVEL<<"time"<<tag.GetEnqueTime()<<endl;

			result = FifoEnqueue(GetPointer(item), departureRound % FIFO_PER_LEVEL);

		}

	}

	else{

		//hasTag == 1

		cout<< "The pkt should be migrated."<<endl;

	   	departureRound = tag.GetDepartureRound();



		//level 2

		if ((departureRound - currentRound) >= (FIFO_PER_LEVEL * FIFO_PER_LEVEL)){

			// Update Pkt Tag

			cout<<"The index before change is "<<tag.GetIndex();

			tag.SetIndex(departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL);

			packet->ReplacePacketTag(tag);

			cout<<"The index after change is "<<tag.GetIndex();

			cout<<"The destination to be stored is:"<<endl;

			cout << "departureRound:" << departureRound << " insertLevel:" << 2 << " index:" << departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL<<endl;

			result = PifoEnqueue(2, GetPointer(item));

		}

		//level 1

		else if ((departureRound - currentRound) >= FIFO_PER_LEVEL){

			cout<<"The index before change is "<<tag.GetIndex();

			tag.SetIndex(departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);

			packet->ReplacePacketTag(tag);

			cout<<"The index after change is "<<tag.GetIndex();

			cout<<"The destination to be stored is:"<<endl;

			cout << "departureRound:" << departureRound << " insertLevel:" << 1 << " index:" << departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL<<endl;

			result = PifoEnqueue(1, GetPointer(item));

		}

		//level 0

		else{   cout<<"The index after change is "<<tag.GetIndex();

			tag.SetIndex(departureRound % FIFO_PER_LEVEL);

			packet->ReplacePacketTag(tag);

			cout<<"The index after change is "<<tag.GetIndex();

			cout<<"The destination to be stored is:"<<endl;

			cout << "departureRound:" << departureRound << " insertLevel:" << 0 << " index:" << departureRound % FIFO_PER_LEVEL<<endl;

	

			result = FifoEnqueue(GetPointer(item), departureRound % FIFO_PER_LEVEL);

			cout<<"result"<<result;	

		}

		

	}

	Record("EnqueuedPktsList", departureRound);

	if (result == true){

	    if(hasTag==0){

		 setPktCount(pktCount + 1);

		 cum_pktCount += 1;

	    }

           

	    cout << "The pkt is enqueued successfully" << endl;

	    cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Enque " << packet << " Pkt:" << packet->GetUid());

            return true;

	}

	else{// fifo overflow drop pkt

	    cout << "The pkt fail to be enqueued" << endl;

	    cout <<"Gearbox DoEnqueue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	    return false;

	}

    }



    bool Gearbox_pl_fid_flex::PifoEnqueue(int level, QueueDiscItem* item){

	cout<<endl;

	cout<<"Gearbox PifoEnque <<<<<<<<<<<<<<<<Start"<<endl;

	Packet* packet = GetPointer(item->GetPacket());

	GearboxPktTag tag;

	packet->PeekPacketTag(tag);

	cout<<" index:" << tag.GetIndex()<<endl;

	item->GetPacket()->PeekPacketTag(tag);

	cout<<"????????????gearbox pifoenque "<<tag.GetPifoenque()<<endl;



 	QueueDiscItem* re = levels[level].pifoEnque(item);



	item->GetPacket()->PeekPacketTag(tag);

	cout<<"!!!!!!!!!!Gearbox pifoenque "<<tag.GetPifoenque()<<endl;



	//when re != 0, the reason is the corresponding fifo is full

	// if overflow, go to higher levels

	if(re != 0){//?if(re != 0)

		// drop if no higher level to enque

		if (level == DEFAULT_VOLUME - 1){

			cout << "DROP!!! no higher level to enque" << endl;

			Drop(re);

			dropCount = dropCount+1;

			setDropCount(dropCount);

			return false; // or true???

		}

		level = level + 1;

		GearboxPktTag tag;

		Packet* re_packet = GetPointer(re->GetPacket());

		re_packet->PeekPacketTag(tag);

		tag.SetIndex(cal_index(level, tag.GetDepartureRound()));

		re_packet->ReplacePacketTag(tag);

		//if the overflow times == 2, drop it; 

		//if the overflow times == 1, increase the overflow times and enque to higher level

		int overflow = tag.GetOverflow();

		cout<<"overflow=="<<overflow<<endl;

		overflow = overflow+1; //

		cout<<"overflow+1=="<<overflow<<endl;

		if(overflow == 2){

			cout<<"Second time overflow"<< endl;

			cout<<"DROP!!! Second time overflow"<< endl;

			Drop(re);

			dropCount = dropCount+1;

			setDropCount(dropCount);

			return false;

		}

		else{   

			cout<<"First time overflow"<< endl;

			cout<<"The pkt is enqueued to level "<< level <<endl;

			tag.SetOverflow(1);

			re_packet->ReplacePacketTag(tag);

			overflow = tag.GetOverflow();

			cout<<"overflow=="<<overflow<<endl;

			re = levels[level].pifoEnque(item);

		}	

	} 

	cout<<"Gearbox PifoEnque <<<<<<<<<<<<<<<<End"<<endl;

	return true;//means level 1's corresponding fifo is full

    }

    bool Gearbox_pl_fid_flex::FifoEnqueue(QueueDiscItem* item, int index){

	cout<<endl;

	cout<<"Gearbox FifoEnqueue <<<<<<<<<<<<<<<<Start"<<endl;

	Packet* packet = GetPointer(item->GetPacket());

	GearboxPktTag tag;

	packet->PeekPacketTag(tag);

	cout<<" index:" << tag.GetIndex()<<endl;

	QueueDiscItem* re = levels[0].fifoEnque(item, index);

	// if overflow, go to higher levels

	int level = 0;

	if(re != 0){//?if(re != 0)

		// drop if no higher level to enque

		if (level >= DEFAULT_VOLUME - 1){

			cout << "DROP!!! no higher level to enque" << endl;

			Drop(re);

			dropCount = dropCount+1;

			setDropCount(dropCount);

			return false; // or true???

		}

		

		GearboxPktTag tag;

		Packet* re_packet = GetPointer(re->GetPacket());

		re_packet->PeekPacketTag(tag);	

		//if the overflow times == 2, drop it; 

		//if the overflow times == 1, increase the overflow times and enque to higher level

		int overflow = tag.GetOverflow();

		overflow += 1; 

		cout<<"overflow+1=="<<overflow<<endl;

		if(overflow == 2){

			cout<<"Second time overflow"<< endl;

			cout<<"DROP!!! Second time overflow"<< endl;

			Drop(re);

			dropCount = dropCount+1;

			setDropCount(dropCount);

			return false;

		}

		else{

			cout<<"First time overflow"<< endl;

			cout<<"The pkt is enqueued to level "<< level <<endl;

			tag.SetOverflow(1);

			level = level + 1;

			tag.SetIndex(cal_index(level, tag.GetDepartureRound()));

			re_packet->ReplacePacketTag(tag);

			overflow = tag.GetOverflow();

			cout<<"overflow=="<<overflow<<endl;

			Packet* packet = GetPointer(item->GetPacket());

			GearboxPktTag tag;

			packet->PeekPacketTag(tag);

			cout<<" overflow:" << tag.GetOverflow()<<endl;

			

			re = levels[level].pifoEnque(item);

		}	

		

	}

	return true;

    }

	

    int Gearbox_pl_fid_flex::cal_index(int level, int departureRound){

	int term = 1;

	for (int i = 0; i < level; i++){

		term *= FIFO_PER_LEVEL;

	}

	//cout << "CalculateIndex:" << departureRound / term % FIFO_PER_LEVEL << endl;

	return departureRound / term % FIFO_PER_LEVEL;

    }



    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {

	cout<<endl;

	cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

        //if (pktCount == 0) {

		//cout<<"The schedulor is empty"<<endl;

		//std::ofstream thr ("GBResult/dropCount.dat", std::ios::out | std::ios::app);

		//thr <<  Simulator::Now().GetSeconds() << " " << getDropCount() << std::endl;

            	//return 0;

       //}

	int earliestLevel = findearliestpacket(DEFAULT_VOLUME);

	if(earliestLevel == -1){

		std::ofstream thr ("GBResult/CountStat.dat", std::ios::out | std::ios::app);

		thr <<  Simulator::Now().GetSeconds() << " drop " << getDropCount() << " enque " << enqueCount<< " deque "<< dequeCount << " overflow " << overflowCount << " reload "<< reloadCount << std::endl;

		for(int i=0; i<int(Flowlist.size()); i++){

			uint64_t flowLabel = Flowlist.at(i);

			Flow_pl* currFlow = getFlowPtr(flowLabel);

			thr <<  Simulator::Now().GetSeconds() << " Flow"<< i<<" completion time: "<< currFlow->getFinishTime() - currFlow->getStartTime() << std::endl;

		}

		thr <<  Simulator::Now().GetSeconds() <<" Pkt queing delay: "<< queuingCount/cum_pktCount << std::endl;

		return NULL;

	}

	Ptr<QueueDiscItem> re = NULL;

	if(earliestLevel == 0){

		

		re = FifoDequeue(0);

	}

	else{

	

		re = PifoDequeue(earliestLevel);

	}

	//Set the finish time of flow

	uint64_t flowlabel = getFlowLabel(re);

        Flow_pl* currFlow = getFlowPtr(flowlabel);

	currFlow->setFinishTime(Simulator::Now().GetSeconds());

	



	GearboxPktTag tag;

	Packet* packet = GetPointer(GetPointer(re)->GetPacket());

	packet->PeekPacketTag(tag);

	tag.SetDequeTime(Simulator::Now().GetSeconds());

	packet->ReplacePacketTag(tag);



	int departureround = tag.GetDepartureRound();

	Record("DequeuedPktsList", departureround);

	PacketRecord("DequeuedPktsList", departureround, re);

	cout <<"Gearbox DoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	return re;

	

   }















	void Gearbox_pl_fid_flex::Record(string fname, int departureRound){

		string path = "GBResult/pktsList/";

		path.append(fname);

		FILE *fp;

		cout<<path<<endl;

		fp = fopen(path.data(), "a+"); //open and write

		//fprintf(fp, "%f", Simulator::Now().GetSeconds());

		fprintf(fp, "%d", departureRound);

		fprintf(fp, "\t");

		fclose(fp);

		FILE *fp2;

		fp2 = fopen("GBResult/pktsList/LevelsSize", "a+"); //open and write

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

		fprintf(fp2, "%s", " pifo:");

		fprintf(fp2, "%d", levels[2].getPifoSize());

		fprintf(fp2, "%s", "\t\t");

		fprintf(fp2, "%s", "GB total:");

		fprintf(fp2, "%d", levels[0].getFifoTotalNPackets() + levels[1].getFifoTotalNPackets() + levels[1].getPifoSize() + levels[2].getFifoTotalNPackets() + levels[2].getPifoSize());

		fprintf(fp2, "\t%s", fname.data());

		fprintf(fp2, "\n");

		fclose(fp2);

		FILE *fpl0;

		

		fpl0 = fopen("GBResult/pktsList/Level0", "a+"); //open and write

		fprintf(fpl0, "%f", Simulator::Now().GetSeconds());

		fprintf(fpl0, "\t%d", levels[0].getFifoTotalNPackets());

		fprintf(fpl0, "\n");

		fclose(fpl0);

		FILE *fpl1;

		fpl1 = fopen("GBResult/pktsList/Level1", "a+"); //open and write

		fprintf(fpl1, "%f", Simulator::Now().GetSeconds());

		fprintf(fpl1, "\t%d", levels[1].getFifoTotalNPackets());

		fprintf(fpl1, "\n");

		fclose(fpl1);

		FILE *fpl2;

		fpl2 = fopen("GBResult/pktsList/Level2", "a+"); //open and write

		fprintf(fpl2, "%f", Simulator::Now().GetSeconds());

		fprintf(fpl2, "\t%d", levels[2].getFifoTotalNPackets());

		fprintf(fpl2, "\n");

		fclose(fpl2);

   }







    void Gearbox_pl_fid_flex::Rate(int enqueCount, int dequeCount, int overflowCount, int reloadCount){

		curTime = Simulator::Now();

		if((curTime.GetSeconds () - prevTime1.GetSeconds ())>1){

			std::ofstream thr1 ("GBResult/enqueRate.dat", std::ios::out | std::ios::app);

			std::ofstream thr2 ("GBResult/dequeRate.dat", std::ios::out | std::ios::app);

			std::ofstream thr3 ("GBResult/overflowRate.dat", std::ios::out | std::ios::app);

			std::ofstream thr4 ("GBResult/reloadRate.dat", std::ios::out | std::ios::app);	

			enquerate = (enqueCount - enque_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 

			thr1 <<  curTime.GetSeconds () << " " << enquerate << std::endl;



			dequerate = (dequeCount - deque_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 

			thr2 <<  curTime.GetSeconds () << " " << dequerate << std::endl;



			overflowrate = (overflowCount - overflow_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 

			thr3 <<  curTime.GetSeconds () << " " << overflowrate << std::endl;



			reloadrate = (reloadCount - reload_previous) / (curTime.GetSeconds () - prevTime1.GetSeconds ()); 		

			thr4 <<  curTime.GetSeconds () << " " << reloadrate << std::endl;

			prevTime1 = curTime;

			enque_previous = enqueCount;

			deque_previous = dequeCount;

			overflow_previous = overflowCount;

			reload_previous = reloadCount;



		}





    }





    void Gearbox_pl_fid_flex::PacketRecord(string fname, int departureRound, Ptr<QueueDiscItem> item){

		//get pkt information

		GearboxPktTag tag;



        	item->GetPacket()->PeekPacketTag(tag);

		int fifoenque = tag.GetFifoenque();

		int fifodeque = tag.GetFifodeque();

		int pifoenque = tag.GetPifoenque();

		int pifodeque = tag.GetPifodeque();

		int overflow = tag.GetOverflow();

		int reload = tag.GetReload();

		double queuingDelay = tag.GetDequeTime() - tag.GetEnqueTime();



		enqueCount += fifoenque + pifoenque;



  		dequeCount += fifodeque + pifodeque;

		

		overflowCount += overflow;



		reloadCount +=  reload;



		queuingCount += queuingDelay;



		Rate(enqueCount,dequeCount,overflowCount,reloadCount);

		



		string path = "GBResult/pktsList/";



		path.append(fname);



		cout<<path<<endl;



		FILE *fp2;



		fp2 = fopen("GBResult/pktsList/Pktdata", "a+"); //open and write



		fprintf(fp2, "%f", Simulator::Now().GetSeconds());



		fprintf(fp2, "\t%s", "depatureRound:");



		fprintf(fp2, "%d", departureRound);



		fprintf(fp2, "\t%s", "fifoenque:");



		fprintf(fp2, "%d", fifoenque);



		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "fifodeque:");



		fprintf(fp2, "%d", fifodeque);

		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "pifoenque:");



		fprintf(fp2, "%d", pifoenque);



		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "pifodeque:");



		fprintf(fp2, "%d", pifodeque);

		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "overflow:");



		fprintf(fp2, "%d", overflow);



		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "reload:");



		fprintf(fp2, "%d", reload);

		

		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "queueing delay:");



		fprintf(fp2, "%lf", queuingDelay);

		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "enquetime:");



		fprintf(fp2, "%lf", tag.GetEnqueTime());

		fprintf(fp2, "%s", "\t");



		fprintf(fp2, "%s", "dequetime:");



		fprintf(fp2, "%lf", tag.GetDequeTime());



		fprintf(fp2, "\n");



		fclose(fp2);



   }







   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::FifoDequeue(int earliestLevel) {

	cout<<endl;

	cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

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

	cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from fifo in level "<< 0 <<endl;

	this->Migration(currentRound);

	cout <<"Gearbox FifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	return p;

   }

   Ptr<QueueDiscItem> Gearbox_pl_fid_flex::PifoDequeue(int earliestLevel) {

	cout<<endl;

	cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Start"<<endl;

   	QueueDiscItem* pifoitem = levels[earliestLevel].pifoDeque();

	GearboxPktTag tag;

	pifoitem->GetPacket()->PeekPacketTag(tag);

	this->setCurrentRound(tag.GetDepartureRound());

	Ptr<QueueDiscItem> p = pifoitem;

	setPktCount(pktCount - 1);

	cout<<"Deque pkt "<<tag.GetDepartureRound()<<" from pifo in level "<<earliestLevel<<endl;

	

	this->Migration(currentRound);

	cout <<"Gearbox PifoDequeue<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<End"<<endl;

	return p;

   }



   void Gearbox_pl_fid_flex::Migration(int currentRound){

	cout<<endl;

	cout<<"Gearbox Migration <<<<<<<<<<<<<<<<Start"<<endl;

	cout<<"Current Round is "<< currentRound<<endl;

	//level 1

	//while(!levels[1].isCurrentFifoEmpty() && (i < SPEEDUP_FACTOR)){

	for(int j = 1; j<3; j++){

		int npkts = levels[j].getCurrentFifoNPackets() < SPEEDUP_FACTOR ? levels[j].getCurrentFifoNPackets() : SPEEDUP_FACTOR;

		for(int i = 0; i < npkts; i++){

			cout<<endl;

	    		cout << "It's the " << i <<" time"<<endl;

			//fifo deque

			Ptr<QueueDiscItem> item = levels[j].fifoDeque(levels[j].getCurrentIndex());

			GearboxPktTag tag;

			Packet* packet = GetPointer(GetPointer(item)->GetPacket());	

			//Get departure round					

			packet->PeekPacketTag(tag);

			int departureRound = tag.GetDepartureRound();

			//enque to scheduler

			cout << "Migrated packet: "<< departureRound <<" from level "<<j<< endl;	

			DoEnqueue(item);

			//get migration times and set

			int migration = tag.GetMigration()+1;

			tag.SetMigration(migration);

			packet->ReplacePacketTag(tag);

		

			cout<<"Migration times is "<<migration;	

		}	



	}

	cout<<"Gearbox Migration <<<<<<<<<<<<<<<<End"<<endl;

   }



    void Gearbox_pl_fid_flex::setDropCount(int count){

	this->dropCount = count;

    }



    int Gearbox_pl_fid_flex::getDropCount(){

	return dropCount;

    }





    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(uint64_t fid, int weight, int brustness) { // Peixuan 04212020

	Flowlist.push_back(fid);

        string key = convertKeyValue(fid);  // Peixuan 04212020

        Flow_pl* newFlowPtr = new Flow_pl(fid, weight, brustness);

	newFlowPtr->setStartTime(Simulator::Now().GetSeconds());

        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));

        return this->flowMap[key];

    }



    int Gearbox_pl_fid_flex::updateFlowPtr(int departureRound, uint64_t fid, Flow_pl* flowPtr) { // Peixuan 04212020

        string key = convertKeyValue(fid);  // Peixuan 04212020

	// update flow info

        flowPtr->setLastFinishRound(departureRound + flowPtr->getWeight());    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped

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

