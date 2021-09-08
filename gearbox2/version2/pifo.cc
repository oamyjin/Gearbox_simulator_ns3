/*
 * this implementation refers to PIFO Queue
 * just like a linkedlist
 *
 * Author: Jiajin
 */
#include "pifo.h"
#include "ns3/log.h"
#include <vector>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("Pifo");
	NS_OBJECT_ENSURE_REGISTERED(Pifo);

	TypeId Pifo::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::Pifo")
			.SetParent<Object>()
			.SetGroupName("TrafficControl")
			.AddConstructor<Pifo>()
			;
		return tid;
	}

	Pifo::Pifo() : Pifo(20, 10) {
		NS_LOG_FUNCTION(this);
	}

	Pifo::Pifo(int h, int l) {
		NS_LOG_FUNCTION(this);
		this->H_value = h;
		this->L_value = l;
		this->max_value = 99999999;
	}

	Pifo::~Pifo() {
		NS_LOG_FUNCTION(this);
	}


	vector<QueueDiscItem*> Pifo::Push(QueueDiscItem* item, int dRound) {
		// insert into the list directly if empty
		vector<QueueDiscItem*> re;
		if (list.size() == 0) {
			list.push_back(item);
			GearboxPktTag tag;
			list.at(0)->GetPacket()->PeekPacketTag(tag);
			GearboxPktTag tag1;
			list.at(list.size() - 1)->GetPacket()->PeekPacketTag(tag1);
		}
		else {
			// find the right position
			int k = 0;
			while (true) {
				// get the departure round
				if (k == (int)list.size()) {
					list.push_back(item); // insert into the end
					break;
				}
				GearboxPktTag tag;
				list.at(k)->GetPacket()->PeekPacketTag(tag);
				int tagValue = tag.GetDepartureRound();
				if (dRound < tagValue || dRound == tagValue){
					list.insert(list.begin() + k, item); //insert into the index of k
					break;
				} 
				k++;
			}
			// return items if pifo's size exceeds H_value, else return 0		
			if (int(list.size()) > this->H_value) {
				while (int(list.size()) < this->H_value) {
					re.push_back(list.back()); // get the last item
					list.pop_back(); // delete the last item
				}
			}
		}
		
		UpdateMaxValue();
		return re;
	}


	QueueDiscItem* Pifo::Pop() {
		if (list.size() == 0) {
			return NULL;
		}
		else {
			QueueDiscItem* popitem = list.front();
			list.erase(list.begin());
			UpdateMaxValue();
			return popitem;
		}
	}

	QueueDiscItem* Pifo::Peek() {
		if (list.size() != 0) {
			return list.front();
		}
		else {
			return NULL;
		}
	}



	void Pifo::UpdateMaxValue() {
		if (list.size() == 0) {
			max_value = 99999999;
		}
		else {
			GearboxPktTag tag;
			list.at(list.size() - 1)->GetPacket()->PeekPacketTag(tag);
			max_value = tag.GetDepartureRound();
		}
	}

	int Pifo::GetMaxValue() {
		return max_value;
	}

	int Pifo::Size() {
		NS_LOG_FUNCTION(this);
		return int(list.size());
	}

	int Pifo::LowestSize() {
		NS_LOG_FUNCTION(this);
		return this->L_value;
	}

	int Pifo::HighestSize() {
		NS_LOG_FUNCTION(this);
		return this->H_value;
	}


	QueueDiscItem* Pifo::PopFromBottom() {
		QueueDiscItem* lastPacket;
		lastPacket = list.back(); // get the last item
		list.pop_back(); // delete the last item
		UpdateMaxValue();
		return lastPacket;
	}

	void Pifo::Print() {
		cout << "Pifo Print:";
		for (int i = 0; i < int(list.size()); i++) {
			PacketTagIterator::Item ititem = list.at(i)->GetPacket()->GetPacketTagIterator().Next();
			Callback<ObjectBase*> constructor = ititem.GetTypeId().GetConstructor();
			ObjectBase* instance = constructor();
			GearboxPktTag* tag = dynamic_cast<GearboxPktTag*> (instance);
			ititem.GetTag(*tag);
			cout << "(" << i << ": " << tag->GetDepartureRound() << ", " << tag->GetIndex() << ") ";
		}
		cout << endl;
	}
}
