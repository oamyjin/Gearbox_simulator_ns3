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

        int tag_min = 99999;

        GearboxPktTag tag;

        for (int i = 1; i < volume; i++) {

            QueueDiscItem* item = levels[i].pifopeek();

            if (item == NULL) {

                continue;

            }

            Packet* packet = GetPointer(item->GetPacket());

            packet->PeekPacketTag(tag);

            int departureRound = tag.GetDepartureRound();



            if ((i == 1) | (departureRound < tag_min)) {

                tag_min = departureRound;

                level_min = i;

            }

        }

        const QueueItem* item = levels[0].fifopeek();

        Packet* packet = GetPointer(item->GetPacket());

        packet->PeekPacketTag(tag);

        int departureRound = tag.GetDepartureRound();

        if (departureRound <= tag_min) { //deque level0 first?

            tag_min = departureRound;

            level_min = 0;

            return 0;

        }

        else {

            return level_min;

        }

    }





    void Gearbox_pl_fid_flex::PrintSomething(const std::string& strInput, const int& nInput)

    {

        std::cout << strInput << nInput << std::endl;

    }





    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {//to be considered

        this->currentRound = currentRound;

    }



    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {

        ///fprintf(stderr, "Set Packet Count: %d\n", pktCount); // Debug: Peixuan 07072019

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

            Drop(item);

            return false;   // 07072019 Peixuan: exceeds the maximum round

        }





        int curBrustness = currFlow->getBrustness();

        if ((departureRound - currentRound) >= curBrustness) {

            Drop(item);

            return false;   // 07102019 Peixuan: exceeds the maximum brustness

        }



        float curWeight = currFlow->getWeight();

        currFlow->setLastFinishRound(departureRound + int(curWeight));    // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped

        this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid





        // LEVEL_2

        if (departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) > 1 || insertLevel == 2) {



            //gearbox2             

            currFlow->setInsertLevel(2);

            this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid

    // Add Pkt Tag

            GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL));

            levels[2].enque(GetPointer(item), departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL, 1);



            // LEVEL_1

        }

        else if (departureRound / FIFO_PER_LEVEL - currentRound / FIFO_PER_LEVEL > 1 || insertLevel == 1) {

            this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid

    // Add Pkt Tag

            GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL));

            currFlow->setInsertLevel(1);

            levels[1].enque(GetPointer(item), departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL, 1);

            // LEVEL_0

        }

        else {

            currFlow->setInsertLevel(0);

            this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid

    // Add Pkt Tag

            GetPointer(item)->GetPacket()->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));

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

        int earliestLevel = findearliestpacket(DEFAULT_VOLUME);



        if (earliestLevel == 0) {

            QueueDiscItem* fifoitem = levels[0].fifoDeque();

            GearboxPktTag tag;

            fifoitem->GetPacket()->PeekPacketTag(tag);

            this->setCurrentRound(tag.GetDepartureRound());

            Ptr<QueueDiscItem> p = fifoitem;

            setPktCount(pktCount - 1);

            return p;

        }

        else {

            QueueDiscItem* pifoitem = levels[earliestLevel].pifoDeque();

            GearboxPktTag tag;

            pifoitem->GetPacket()->PeekPacketTag(tag);

            this->setCurrentRound(tag.GetDepartureRound());

            Ptr<QueueDiscItem> p = pifoitem;

            setPktCount(pktCount - 1);

            return p;



        }



    }



    // Peixuan: now we only call this function to get the departure packet in the next round

    vector<Ptr<QueueDiscItem>> Gearbox_pl_fid_flex::runRound() {//to be considered : using it to update the current index

        vector<Ptr<QueueDiscItem>> result;



        // Debug: Peixuan 07062019: Bug Founded: What if the queue is empty at the moment? Check Size!



        //current round done



        vector<Ptr<QueueDiscItem>> upperLevelPackets = serveUpperLevel(currentRound);



        result.insert(result.end(), upperLevelPackets.begin(), upperLevelPackets.end());

        QueueDiscItem* p = levels[0].fifoDeque();



        if (!p) {

        }



        while (p) {

            Ipv6Header iph;

            GetPointer(p->GetPacket())->PeekHeader(iph);





            result.push_back(Ptr<QueueDiscItem>(p));

            p = levels[0].fifoDeque();

        }



        levels[0].getAndIncrementIndex();               // Level 0 move to next FIFO



        // 01052020 Peixuan

        bool is_level_1_update = false;

        bool is_level_2_update = false;



        if (levels[0].getCurrentIndex() == 0) {

            is_level_1_update = true;

        }



        if (is_level_1_update) {

            levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO

            if (levels[1].getCurrentIndex() == 0) {

                is_level_2_update = true;

            }

        }

        if (is_level_2_update) {

            levels[2].getAndIncrementIndex();            // Level 3 move to next FIFO

        }



        return result;

    }



    //Peixuan: This is also used to get the packet served in this round (VC unit)

    // We need to adjust the order of serving: level0 -> level1 -> level2

    vector<Ptr<QueueDiscItem>> Gearbox_pl_fid_flex::serveUpperLevel(int currentRound) {



        vector<Ptr<QueueDiscItem>> result;



        if (!levels[1].isCurrentFifoEmpty()) {

            int size = static_cast<int>(ceil(levels[1].getCurrentFifoSize() * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)

            for (int i = 0; i < size; i++) {

                QueueDiscItem* p = levels[1].fifoDeque();

                if (p == 0)

                    break;

                Ipv6Header iph;

                GetPointer(p->GetPacket())->PeekHeader(iph);

                result.push_back(Ptr<QueueDiscItem>(p));

            }

        }

        // then level 2

        if (!levels[2].isCurrentFifoEmpty()) {

            int size = static_cast<int>(ceil(levels[2].getCurrentFifoSize() * 1.0 / ((FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound % (FIFO_PER_LEVEL * FIFO_PER_LEVEL))));

            for (int i = 0; i < size; i++) {

                QueueDiscItem* p = levels[2].fifoDeque();

                if (p == 0)

                    break;

                Ipv6Header iph;

                GetPointer(p->GetPacket())->PeekHeader(iph);

                result.push_back(Ptr<QueueDiscItem>(p));

            }

        }

        return result;

    }





    // 12132019 Peixuan

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