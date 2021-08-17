#include "ns3/Level_flex.h"
#include "ns3/Flow_pl.h"
#include <cmath>
#include <sstream>
#include <vector>
#include <string>

#include <map>

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
        //NS_LOG_FUNCTION (this);
        //fprintf(stderr, "Created new HCSPL instance\n");
    }

    Gearbox_pl_fid_flex::Gearbox_pl_fid_flex(int volume) {
        NS_LOG_FUNCTION(this);

        //fprintf(stderr, "Created new HCSPL instance with volumn = %d\n", volume); // Debug: Peixuan 07062019
        this->volume = volume;
        currentRound = 0;
        pktCount = 0; // 07072019 Peixuan
        //pktCurRound = new vector<Packet*>;
        //12132019 Peixuan
        typedef std::map<string, Flow_pl*> FlowMap;
        FlowMap flowMap;
        //cout << "initialized"<<endl;
    }

    void Gearbox_pl_fid_flex::PrintSomething(const std::string& strInput, const int& nInput)
    {
        std::cout << strInput << nInput << std::endl;
    }


    void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {
        this->currentRound = currentRound;

        level0ServingB = ((int)(currentRound / FIFO_PER_LEVEL) % 2);
        level1ServingB = ((int)(currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL)) % 2);
    }

    void Gearbox_pl_fid_flex::setPktCount(int pktCount) {
        ///fprintf(stderr, "Set Packet Count: %d\n", pktCount); // Debug: Peixuan 07072019
        this->pktCount = pktCount;
    }


    bool Gearbox_pl_fid_flex::DoEnqueue(Ptr<QueueDiscItem> item) {


        NS_LOG_FUNCTION(this);
        Packet* packet = GetPointer(GetPointer(item)->GetPacket());

        //modify 
        Ipv6Header iph;
        packet->PeekHeader(iph);
        //Ipv6Header *ipv6Header = dynamic_cast<Ipv6Header*> (iph);
        int pkt_size = packet->GetSize();

        int departureRound = cal_theory_departure_round(iph, pkt_size);
        
        string key = convertKeyValue(iph.GetFlowLabel()); // Peixuan 04212020 fid
        // Not find the current key
        if (flowMap.find(key) == flowMap.end()) {
            this->insertNewFlowPtr(iph.GetFlowLabel(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020 fid
        }


        Flow_pl* currFlow = flowMap[key];
        int insertLevel = currFlow->getInsertLevel();

        departureRound = max(departureRound, currentRound);

        if ((departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {
            ///fprintf(stderr, "?????Exceeds maximum round, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
            Drop(item);
            return false;   // 07072019 Peixuan: exceeds the maximum round
        }


        //int curFlowID = convertKeyValue(iph->saddrm, iph->daddr)   // use source IP as flow id
        int curBrustness = currFlow->getBrustness();
        if ((departureRound - currentRound) >= curBrustness) {
            ///fprintf(stderr, "?????Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
            Drop(item);
            return false;   // 07102019 Peixuan: exceeds the maximum brustness
        }

        currFlow->setLastDepartureRound(departureRound);     // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
        //this->updateFlowPtr(iph->saddr(), iph->daddr(), currFlow);  //12182019 Peixuan
        this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid

        ///fprintf(stderr, "At Round: %d, Enqueue Packet from Flow %d with Finish time = %d.\n", currentRound, iph->saddr(), departureRound); // Debug: Peixuan 07072019

        int level0InsertingB = ((int)(departureRound / FIFO_PER_LEVEL) % 2);
        int level1InsertingB = ((int)(departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL)) % 2);

        

        // LEVEL_2
        if (departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) > 1 || insertLevel == 2) {
            //gearbox2 
            //compare the packet priority with pifo largest pririty
            GearboxPktTag tag;
	    item->GetPacket()->PeekPacketTag(tag);
	    int tagValue = tag.GetDepartureRound();
            if (tagValue < levels[2].getPifoMaxValue())
            {
                levels[2].pifoEnque(GetPointer(item));
            }
            else
            {
                if (departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                    currFlow->setInsertLevel(1);
                    //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                    this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
                    hundredLevel.fifoEnque(GetPointer(item), departureRound / (FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                    ///fprintf(stderr, "Enqueue Level 3, third FIFO, fifo %d\n", departureRound / (4*4) % 4); // Debug: Peixuan 07072019
                }
                else {
                    currFlow->setInsertLevel(2);
                    //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                    this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
                    levels[2].fifoEnque(GetPointer(item), departureRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                    ///fprintf(stderr, "Enqueue Level 3, regular FIFO, fifo %d\n", departureRound / (4*4*4) % 4); // Debug: Peixuan 07072019
                }
            }

            // LEVEL_1
        }
        else if (departureRound / FIFO_PER_LEVEL - currentRound / FIFO_PER_LEVEL > 1 || insertLevel == 1) {
            if (!level1InsertingB) {
                /////fprintf(stderr, "Enqueue Level 1\n"); // Debug: Peixuan 07072019
                if (departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                    currFlow->setInsertLevel(0);
                    //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                    this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid    
		    // Add Pkt Tag
   		    GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));
                    decadeLevel.enque(GetPointer(item), departureRound % FIFO_PER_LEVEL);
                    ///fprintf(stderr, "Enqueue Level 1, decede FIFO, fifo %d\n", departureRound  % 4); // Debug: Peixuan 07072019
                }
                else {
                    currFlow->setInsertLevel(1);
                    //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                    this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
		    // Add Pkt Tag
   		    GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL));
                    levels[1].enque(GetPointer(item), departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
                    ///fprintf(stderr, "Enqueue Level 1, regular FIFO, fifo %d\n", departureRound / 4 % 4); // Debug: Peixuan 07072019
                }
            }
            else {
                /////fprintf(stderr, "Enqueue Level B 1\n"); // Debug: Peixuan 07072019
                if (departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                    currFlow->setInsertLevel(0);
                    //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                    this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
		    // Add Pkt Tag
   		    GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));
                    decadeLevelB.enque(GetPointer(item), departureRound % FIFO_PER_LEVEL);
                    ///fprintf(stderr, "Enqueue Level B 1, decede FIFO, fifo %d\n", departureRound  % 4); // Debug: Peixuan 07072019
                }
                else {
                    currFlow->setInsertLevel(1);
                    //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                    this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
		    // Add Pkt Tag
   		    GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL));
                    levelsB[1].enque(GetPointer(item), departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
                    ///fprintf(stderr, "Enqueue Level B 1, regular FIFO, fifo %d\n", departureRound / 4 % 4); // Debug: Peixuan 07072019
                }
            }
            // LEVEL_0
        }
        else {
            if (!level0InsertingB) {
                /////fprintf(stderr, "Enqueue Level 0\n"); // Debug: Peixuan 07072019
                currFlow->setInsertLevel(0);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
		// Add Pkt Tag
		cout << "departureRound:" << departureRound << " index:" << (departureRound % FIFO_PER_LEVEL) << " item:" << GetPointer(item) << endl;    
   		GetPointer(item)->GetPacket()->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));
                levels[0].enque(GetPointer(item), departureRound % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 0, regular FIFO, fifo %d\n", departureRound % 4); // Debug: Peixuan 07072019
            }
            else {
                /////fprintf(stderr, "Enqueue Level B 0\n"); // Debug: Peixuan 07072019
                currFlow->setInsertLevel(0);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph.GetFlowLabel(), currFlow);  // Peixuan 04212020 fid
		// Add Pkt Tag
   		GetPointer(GetPointer(item)->GetPacket())->AddPacketTag(GearboxPktTag(departureRound, departureRound % FIFO_PER_LEVEL));
                levelsB[0].enque(GetPointer(item), departureRound % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level B 0, regular FIFO, fifo %d\n", departureRound % 4); // Debug: Peixuan 07072019
            }

        }

        setPktCount(pktCount + 1);
        ///fprintf(stderr, "Packet Count ++\n");

        ///fprintf(stderr, "Finish Enqueue\n"); // Debug: Peixuan 07062019

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

        ///fprintf(stderr, "$$$$$Calculate Departure Round at VC = %d\n", currentRound); // Debug: Peixuan 07062019

        //int curFlowID = iph->saddr();   // use source IP as flow id
        //int curFlowID = iph->flowid();   // use flow id as flow id
        //string key = convertKeyValue(iph->saddr(), iph->daddr());
        string key = convertKeyValue(iph.GetFlowLabel());    // Peixuan 04212020 fid
        //Flow_pl currFlow = *flowMap[key];   // 12142019 Peixuan: We have problem here.
        //Flow_pl currFlow = Flow_pl(1, 2, 100);   // 12142019 Peixuan: Debug
        //Flow_pl* currFlow = this->getFlowPtr(iph->saddr(), iph->daddr()); //12142019 Peixuan fixed
        Flow_pl* currFlow = this->getFlowPtr(iph.GetFlowLabel()); // Peixuan 04212020 fid



        float curWeight = currFlow->getWeight();
        int curLastDepartureRound = currFlow->getLastDepartureRound();
	// update the flow's last departure Round
	currFlow->setLastDepartureRound(int(curLastDepartureRound + curWeight));
        int curStartRound = max(currentRound, curLastDepartureRound);
        //int curDeaprtureRound = (int)(curStartRound + curWeight);
	int curDeaprtureRound = curStartRound;

        return curDeaprtureRound;
    }

    //06262019 Peixuan deque function of Gearbox:

    //06262019 Static getting all the departure packet in this virtual clock unit (JUST FOR SIMULATION PURPUSE!)

    Ptr<QueueDiscItem> Gearbox_pl_fid_flex::DoDequeue() {
        ///fprintf(stderr, "Start Dequeue\n"); // Debug: Peixuan 07062019

        /////fprintf(stderr, "Queue size: %d\n",pktCurRound.size()); // Debug: Peixuan 07062019
        if (pktCount == 0) {
            ///fprintf(stderr, "Scheduler Empty\n"); // Debug: Peixuan 07062019
            return 0;
        }


        //int i = 0; // 07252019 loop debug

        while (!pktCurRound.size()) {   // 07252019 loop debug
        //while (!pktCurRound.size() && i < 1000) {   // 07252019 loop debug
            ///fprintf(stderr, "Empty Round\n"); // Debug: Peixuan 07062019
            pktCurRound = this->runRound();
            ///fprintf(stderr, "Current Round: %d, pkts: %d\n", currentRound, pktCurRound); // Peixuan 01072020
            ///fprintf(stderr, "Current Round: %d, pkts num: %d\n", currentRound, pktCurRound.size()); // Peixuan 01072020
            this->setCurrentRound(currentRound + 1); // Update system virtual clock

            level0ServingB = ((int)(currentRound / FIFO_PER_LEVEL) % 2);
            level1ServingB = ((int)(currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL)) % 2);
            //level2ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
            //level3ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

            ///fprintf(stderr, "Now update Round: %d, level 0 serving B = %d, level 1 serving B = %d, level 2 serving B = %d, level 3 serving B = %d.\n", currentRound, level0ServingB, level1ServingB, level2ServingB, level3ServingB); // Debug: Peixuan 07062019
            //this->deque();
            //i++; // 07252019 loop debug
            //if (i > 1000) { // 07252019 loop debug
            //    return 0;   // 07252019 loop debug
            //}   // 07252019 loop debug
        }

        Ptr<QueueDiscItem> p = pktCurRound.front();
        pktCurRound.erase(pktCurRound.begin());
        setPktCount(pktCount - 1);
        ///fprintf(stderr, "Packet Count --\n");
        Ipv6Header iph;
        GetPointer(p->GetPacket())->PeekHeader(iph);


        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " Deque " << GetPointer(p->GetPacket()) << " Pkt:" << GetPointer(p->GetPacket())->GetUid() << " toString:" << GetPointer(p->GetPacket())->ToString());

        return p;


    }

    // Peixuan: now we only call this function to get the departure packet in the next round
    vector<Ptr<QueueDiscItem>> Gearbox_pl_fid_flex::runRound() {
        vector<Ptr<QueueDiscItem>> result;

        // Debug: Peixuan 07062019: Bug Founded: What if the queue is empty at the moment? Check Size!

        //current round done

        vector<Ptr<QueueDiscItem>> upperLevelPackets = serveUpperLevel(currentRound);

        // Peixuan
        /*for (int i = 0; i < upperLevelPackets.size(); i++) {
            packetNumRecord.push_back(packetNum);
            packetNum--;
        }*/

        result.insert(result.end(), upperLevelPackets.begin(), upperLevelPackets.end());
        /////fprintf(stderr, "Extracting packet\n"); // Debug: Peixuan 07062019
        if (!level0ServingB) {
            QueueDiscItem* p = levels[0].deque();
            /////fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

            if (!p) {
                ///fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
            }

            while (p) {
                Ipv6Header iph;
                GetPointer(p->GetPacket())->PeekHeader(iph);
                //hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 0: fifo %d\n", currentRound, iph->saddr(), levels[0].getCurrentIndex()); // Debug: Peixuan 07092019

                result.push_back(Ptr<QueueDiscItem>(p));
                p = levels[0].deque();
            }

            levels[0].getAndIncrementIndex();               // Level 0 move to next FIFO
            ///fprintf(stderr, "<<<<<At Round:%d, Level 0 update current FIFO as: fifo %d\n", currentRound, levels[0].getCurrentIndex()); // Debug: Peixuan 07212019

            // 01052020 Peixuan
            bool is_level_1_update = false;
            bool is_level_2_update = false;
            //bool is_level_3_update = false;
            //bool is_level_4_update = false;

            if (levels[0].getCurrentIndex() == 0) {
                is_level_1_update = true;
            }

            if (is_level_1_update && !level1ServingB) {
                levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                if (levels[1].getCurrentIndex() == 0) {
                    is_level_2_update = true;
                }
            }

            if (is_level_1_update && level1ServingB) {
                levelsB[1].getAndIncrementIndex();           // Level B 1 move to next FIFO
                if (levelsB[1].getCurrentIndex() == 0) {
                    is_level_2_update = true;
                }
            }


            if (is_level_2_update) {
                levels[2].getAndIncrementIndex();            // Level 3 move to next FIFO
            }

            
        }
        else {
            QueueDiscItem* p = levelsB[0].deque();
            /////fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

            if (!p) {
                ///fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
            }

            while (p) {
                Ipv6Header iph;
                GetPointer(p->GetPacket())->PeekHeader(iph);

                //hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 0: fifo %d\n", currentRound, iph->saddr(), levelsB[0].getCurrentIndex()); // Debug: Peixuan 07092019

                result.push_back(Ptr<QueueDiscItem>(p));
                p = levelsB[0].deque();
            }

            levelsB[0].getAndIncrementIndex();               // Level 0 move to next FIFO
            ///fprintf(stderr, "<<<<<At Round:%d, Level B 0 update current FIFO as: fifo %d\n", currentRound, levelsB[0].getCurrentIndex()); // Debug: Peixuan 07212019


            // 01052020 Peixuan
            bool is_level_1_update = false;
            bool is_level_2_update = false;
            //bool is_level_3_update = false;
            //bool is_level_4_update = false;

            if (levelsB[0].getCurrentIndex() == 0) {
                is_level_1_update = true;
            }

            if (is_level_1_update && !level1ServingB) {
                levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                if (levels[1].getCurrentIndex() == 0) {
                    is_level_2_update = true;
                }
            }

            if (is_level_1_update && level1ServingB) {
                levelsB[1].getAndIncrementIndex();           // Level B 1 move to next FIFO
                if (levelsB[1].getCurrentIndex() == 0) {
                    is_level_2_update = true;
                }
            }

            if (is_level_2_update) {
                levels[2].getAndIncrementIndex();            // Level 3 move to next FIFO
            }

        }


        return result;
    }

    //Peixuan: This is also used to get the packet served in this round (VC unit)
    // We need to adjust the order of serving: level0 -> level1 -> level2
    vector<Ptr<QueueDiscItem>> Gearbox_pl_fid_flex::serveUpperLevel(int currentRound) {
        ///fprintf(stderr, "Serving Upper Level\n"); // Debug: Peixuan 07062019

        vector<Ptr<QueueDiscItem>> result;

        //int level0size = 0;
        int level1size = 0;
        // TODO  level2size NEVERU BE USED
        //int level2size = 0;
        //int level3size = 0;
        //int level4size = 0;

        // TODO  decadelevelsize NEVERU BE USED
        //int decadelevelsize = 0;
        int hundredlevelsize = 0;
        //int thirdlevelsize = 0;
        //int forthlevelsize = 0;

        if (!level1ServingB) {
            level1size = levels[1].getCurrentFifoSize();
        }
        else {
            level1size = levelsB[1].getCurrentFifoSize();
        }

       
        hundredlevelsize = hundredLevel.getCurrentFifoSize();


        //Then: first level 1
        if (currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {

            if (!level1ServingB) {
                int size = decadeLevel.getCurrentFifoSize();
                ///fprintf(stderr, ">>>>>At Round:%d, Serve Level 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevel.getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    QueueDiscItem* p = decadeLevel.deque();
                    if (p == 0)
                        break;
                    result.push_back(Ptr<QueueDiscItem>(p));

                    Ipv6Header iph;
                    GetPointer(p->GetPacket())->PeekHeader(iph);
                }
                decadeLevel.getAndIncrementIndex();
            }
            else {
                // serving backup decade level
                int size = decadeLevelB.getCurrentFifoSize();
                ///fprintf(stderr, ">>>>>At Round:%d, Serve Level B 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevelB.getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    QueueDiscItem* p = decadeLevelB.deque();
                    if (p == 0)
                        break;
                    result.push_back(Ptr<QueueDiscItem>(p));
                    Ipv6Header iph;
                    GetPointer(p->GetPacket())->PeekHeader(iph);

                }
                decadeLevelB.getAndIncrementIndex();
            }

        }
        else {
            if (!level1ServingB) {
                if (!levels[1].isCurrentFifoEmpty()) {
                    int size = static_cast<int>(ceil(levels[1].getCurrentFifoSize() * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                    for (int i = 0; i < size; i++) {
                        QueueDiscItem* p = levels[1].deque();
                        if (p == 0)
                            break;
                        Ipv6Header iph;
                        GetPointer(p->GetPacket())->PeekHeader(iph);
                        result.push_back(Ptr<QueueDiscItem>(p));
                    }
                }
            }
            else {
                if (!levelsB[1].isCurrentFifoEmpty()) {
                    int size = static_cast<int>(ceil(levelsB[1].getCurrentFifoSize() * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));
		    for (int i = 0; i < size; i++) {
                        QueueDiscItem* p = levelsB[1].deque();
                        if (p == 0)
                            break;
                        Ipv6Header iph;
                        GetPointer(p->GetPacket())->PeekHeader(iph);
			result.push_back(Ptr<QueueDiscItem>(p));
                    }
                }

            }
        }

        // then level 2
        if (currentRound / (FIFO_PER_LEVEL * FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
           
            int size = 0;
            size = static_cast<int>(ceil((hundredlevelsize + level1size) * 1.0 / ((FIFO_PER_LEVEL)-currentRound % (FIFO_PER_LEVEL))));  // 01132021 Peixuan *** Fix Level 2 serving order (fixed)

            for (int i = 0; i < size; i++) {
                QueueDiscItem* p = hundredLevel.deque();
                if (p == 0){
                    break;
	        }
                Ipv6Header iph;
                GetPointer(p->GetPacket())->PeekHeader(iph);
                result.push_back(Ptr<QueueDiscItem>(p));
            }
            if (currentRound % (FIFO_PER_LEVEL) == (FIFO_PER_LEVEL)-1){
                hundredLevel.getAndIncrementIndex();
	    }
        }
        else if (!levels[2].isCurrentFifoEmpty()) {
            int size = static_cast<int>(ceil(levels[2].getCurrentFifoSize() * 1.0 / ((FIFO_PER_LEVEL * FIFO_PER_LEVEL) - currentRound % (FIFO_PER_LEVEL * FIFO_PER_LEVEL))));
            for (int i = 0; i < size; i++) {
                QueueDiscItem* p = levels[2].deque();
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
    //Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(nsaddr_t saddr, nsaddr_t daddr) {
    Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(int fid) {

        string key = convertKeyValue(fid);  // Peixuan 04212020
        Flow_pl* flow;
        if (flowMap.find(key) == flowMap.end()) {
            //flow = this->insertNewFlowPtr(saddr, daddr, 2, 100);
            //flow = this->insertNewFlowPtr(saddr, daddr, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
            flow = this->insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020
        }
        flow = this->flowMap[key];
        return flow;
    }

    //Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness) {
    Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020
        //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
        //string key = convertKeyValue(saddr, daddr);
        string key = convertKeyValue(fid);  // Peixuan 04212020
        Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
        //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
        this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
        //flowMap.insert(pair(key, newFlowPtr));
        //return 0;
        return this->flowMap[key];
    }

    //int Gearbox_pl_fid_flex::updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr) {
    int Gearbox_pl_fid_flex::updateFlowPtr(int fid, Flow_pl* flowPtr) { // Peixuan 04212020
        //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
        //string key = convertKeyValue(saddr, daddr);
        string key = convertKeyValue(fid);  // Peixuan 04212020
        //Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
        //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
        this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));
        //flowMap.insert(pair(key, newFlowPtr));
        //return 0;
        return 0;
    }

    //string Gearbox_pl_fid_flex::convertKeyValue(nsaddr_t saddr, nsaddr_t daddr) {
    string Gearbox_pl_fid_flex::convertKeyValue(int fid) { // Peixuan 04212020
        stringstream ss;
        //ss << saddr;
        //ss << ":";
        //ss << daddr;
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

    /*
      find out whether enque into Pifo or Fifo
      return true if Pifo, else Fifo
    */
    bool Gearbox_pl_fid_flex::earliestPacketInPifo(int level){
        // TODO modiy the type of servingB of int to be a vector/list
	// TODO check the methods of pifo
	if (level == 0){
	    if ((level0ServingB == true && levels[level].getFifoTopValue() > levels[level].getPifoMaxValue()) || 
		(level0ServingB == false && levels[level].getFifoTopValue() > levels[level].getPifoMaxValue())){
		return false;
	    }
	    else {
		return true;
    	    }
        }
	else if (level == 1){
	    if ((level1ServingB == true && levelsB[level].getFifoTopValue() > levels[level].getPifoMaxValue()) || 
		(level1ServingB == false && levels[level].getFifoTopValue() > levels[level].getPifoMaxValue())){
		return false;
	    }
	    else {
		return true;
    	    }
        }
        else if (level == 2){
	    if (levels[level].getFifoTopValue() > levels[level].getPifoMaxValue()){
		return false;
	    }
	    else {
		return true;
    	    }
        }
	return false;
    }
}
