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


	QueueDiscItem* Pifo::Push(QueueDiscItem* item, int dRound) {
		// insert into the list directly if empty
		QueueDiscItem* re = NULL;
		cout << "PIFO list.size():" << list.size();
		if (list.size() == 0) {
			list.push_back(item);
			UpdateMaxValue();
			return NULL;
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
			// return the last item if pifo's size exceeds H_value, else return 0	
			cout << "int(list.size()):" << int(list.size()) << " this->H_value:" << this->H_value << endl;
			if (int(list.size()) > this->H_value) {
				cout << "in " << endl;
				re = list.at(list.size() - 1);
				GearboxPktTag tag1;
				list.at(list.size() - 1)->GetPacket()->PeekPacketTag(tag1);
				cout << " list.at(list.size() - 1).dp:" << tag1.GetDepartureRound() << endl; 
				GearboxPktTag tag2;
				re->GetPacket()->PeekPacketTag(tag2);
				cout << " re.dp:" << tag2.GetDepartureRound() << endl;
				list.pop_back();
			}
			UpdateMaxValue();
			cout << "PIFO end re " << re << endl;
			return re;
		}
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
		//cout << "UpdateMaxValue list.size():" << list.size() << endl;
		if (list.size() == 0) {
			max_value = 99999999;
		}
		else {
			GearboxPktTag tag;
			list.at(list.size() - 1)->GetPacket()->PeekPacketTag(tag);
			//cout << "tag.GetDepartureRound():" << tag.GetDepartureRound() << endl;
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
