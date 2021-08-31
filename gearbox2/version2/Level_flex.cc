#include "ns3/object-factory.h"

#include "ns3/Level_flex.h"

#include "ns3/packet.h"

#include "ns3/ptr.h"

#include "ns3/log.h"

#include "ns3/pfifo-fast-queue-disc.h"



namespace ns3 {



    NS_LOG_COMPONENT_DEFINE("Level_flex");



    NS_OBJECT_ENSURE_REGISTERED(Level_flex);



    TypeId Level_flex::GetTypeId(void)

    {

        static TypeId tid = TypeId("ns3::Level_flex")

            .SetParent<Object>()

            .SetGroupName("TrafficControl")

            .AddConstructor<Level_flex>()

            ;

        return tid;

    }



    Level_flex::Level_flex() : Level_flex(DEFAULT_VOLUME, 0) {

        NS_LOG_FUNCTION(this);

    }



    Level_flex::Level_flex(int vol, int index) {

        NS_LOG_FUNCTION(this);

        this->volume = vol;

        this->currentIndex = index;

        this->pifo = Pifo(H_value, L_value);



        ObjectFactory factory;

        factory.SetTypeId("ns3::DropTailQueue");

        factory.Set("Mode", EnumValue(Queue::QUEUE_MODE_PACKETS));

        factory.Set("MaxPackets", UintegerValue(2));



        for (int i = 0; i < volume; i++) {

            fifos[i] = GetPointer(factory.Create<Queue>());

            uint32_t maxSize = 100;

            fifos[i]->SetMaxPackets(maxSize);

        }

    }



    Level_flex::~Level_flex() {

        NS_LOG_FUNCTION(this);

    }





    //gearbox2



    void Level_flex::enque(QueueDiscItem* item, int index) {

        GearboxPktTag tag1;

        item->GetPacket()->PeekPacketTag(tag1);

        GearboxPktTag tag2;

        item->GetPacket()->PeekPacketTag(tag2);



        this->pifoEnque(item);

        //cout << "getPifoMaxValue: " << getPifoMaxValue() << endl;

    }







    void Level_flex::enque(QueueDiscItem* item, int index, bool isPifoEnque) {

        GearboxPktTag tag1;

        item->GetPacket()->PeekPacketTag(tag1);

        GearboxPktTag tag2;

        item->GetPacket()->PeekPacketTag(tag2);

        if (isPifoEnque == 1) {

            this->pifoEnque(item);

        }

        else {

            this->fifoEnque(item, index);

        }

    }



    void Level_flex::fifoEnque(QueueDiscItem* item, int index) {

        //level 0 or the pifo overflow to fifo

        NS_LOG_FUNCTION(this);

        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

        fifos[index]->Enqueue(Ptr<QueueDiscItem>(item));

        pkt_cnt++;

    }



    //gearbox2

    void Level_flex::pifoEnque(QueueDiscItem* item) {

        NS_LOG_FUNCTION(this);

        // get the tag of item

        GearboxPktTag tag;

        item->GetPacket()->PeekPacketTag(tag);

        int departureRound = tag.GetDepartureRound();

        // enque into pifo, if havePifo && ( < maxValue || < L)

        if (departureRound < getPifoMaxValue() || pifo.Size() < pifo.LowestSize()) {

            vector<QueueDiscItem*> re = pifo.Push(item, departureRound);

            while (re.size() != 0) {

                QueueDiscItem* reItem = re.back();

                GearboxPktTag tag;

                reItem->GetPacket()->PeekPacketTag(tag);

                fifoEnque(reItem, tag.GetIndex()); // get the last item

                re.pop_back(); // delete the last item

            }

        }

        // enque into fifo     

        else {

            fifoEnque(item, tag.GetIndex());

        }

        ifLowerThanLthenReload();

    }





    QueueDiscItem* Level_flex::fifoDeque() {

        if (isCurrentFifoEmpty()) {

            return 0;

        }

        Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem>(fifos[currentIndex]->Dequeue());

        pkt_cnt--;

        return GetPointer(item);

    }



    QueueDiscItem* Level_flex::pifoDeque() {

        NS_LOG_FUNCTION(this);

        Ptr<QueueDiscItem> item = pifo.Pop();

        ifLowerThanLthenReload();

        return GetPointer(item);

    }





    void Level_flex::ifLowerThanLthenReload() {

        while (pifo.Size() < pifo.LowestSize()) {

            int earliestFifo = getEarliestFifo();

            if (earliestFifo == -1) {

                break;

            }

            setCurrentIndex(earliestFifo);

            while (!isCurrentFifoEmpty()) {

                pifoEnque(fifoDeque());

                if (pifo.Size() > pifo.HighestSize()) {

                    QueueDiscItem* lastPacket = pifo.PopFromBottom();//error

                    GearboxPktTag tag;

                    lastPacket->GetPacket()->PeekPacketTag(tag);

                    fifoEnque(lastPacket, tag.GetIndex());

                }

            }

        }

    }



    int Level_flex::getEarliestFifo() {

        for (int i = 0; i < volume; i++) {//error ??? to be considered: set current index?

            if (!fifos[(i + currentIndex) % 8]->IsEmpty()) {//to be considered:modify

                return (i + currentIndex) % 8;

            }

        }

        return -1;

    }





    const QueueItem* Level_flex::fifopeek() {

        currentIndex = getEarliestFifo();

        if (isCurrentFifoEmpty()) {

            //fprintf(stderr, "No packet in the current serving FIFO\n"); // Debug: Peixuan 07062019

            return 0;

        }







        Ptr<const QueueItem> item = fifos[currentIndex]->Peek();



        return GetPointer(item);

    }





    QueueDiscItem* Level_flex::pifopeek() {

        return pifo.Peek();

    }











    int Level_flex::getCurrentIndex() {

        return currentIndex;

    }



    void Level_flex::setCurrentIndex(int index) {

        currentIndex = index;

    }



    void Level_flex::getAndIncrementIndex() {

        if (currentIndex + 1 < volume) {

            currentIndex++;

        }

        else {

            currentIndex = 0;

        }

    }



    bool Level_flex::isCurrentFifoEmpty() {

        //fprintf(stderr, "Checking if the FIFO is empty\n"); // Debug: Peixuan 07062019

        //fifos[currentIndex]->length() == 0;

        //fprintf(stderr, "Bug here solved\n"); // Debug: Peixuan 07062019

        return fifos[currentIndex]->IsEmpty();

    }



    int Level_flex::getCurrentFifoSize() {

        return fifos[currentIndex]->GetNBytes();

    }



    int Level_flex::size() {

        // get fifo number

        //return fifos.size();

        return sizeof(fifos) / sizeof(fifos[0]);

    }



    int Level_flex::get_level_pkt_cnt() {

        // get pkt_cntqs

        return pkt_cnt;

    }



    int Level_flex::getPifoMaxValue() {

        return pifo.GetMaxValue();

    }



    int Level_flex::getFifoTopValue() {

        int i = 0;

        int result = 0;

        while (true) {

            if (fifos[i]->Peek() != 0) {

                GearboxPktTag tag;

                GetPointer(fifos[i]->Peek())->GetPacket()->PeekPacketTag(tag);

                result = tag.GetDepartureRound();

                break;

            }

            // if there is no pkt in this level's fifos

            if (i == FIFO_PER_LEVEL - 1) {

                break;

            }

            i++;

        }

        return result;

    }

}