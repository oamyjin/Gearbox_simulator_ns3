/*
 * this implementation refers to PIFO Queue
 * just like a linkedlist
 *
 * Author: Jiajin
 */
#include "pifo.h"
#include "ns3/log.h"
#include <vector>
#include "ns3/Replace_string.h"
#include "ns3/simulator.h"
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

	Pifo::Pifo() : Pifo(20,10) {
		NS_LOG_FUNCTION(this);
	}

	Pifo::Pifo(int h, int l) {
		NS_LOG_FUNCTION(this);
		this->H_value = h;
		this->L_value = l;
		this->max_value = DEFAULT_MAX;
		this->isUpdateMaxValid = 1;
	}

	Pifo::~Pifo() {
		NS_LOG_FUNCTION(this);
	}


	QueueDiscItem* Pifo::Push(QueueDiscItem* item, int dRound) {
		// insert into the list directly if empty
		QueueDiscItem* re = NULL;	
		
		GearboxPktTag tag;
        	item->GetPacket()->PeekPacketTag(tag);

		if (list.size() == 0) {
			list.push_back(item);
			pifoEnque = pifoEnque + 1;
		}
		else {
			// find the right position
			int k = 0;
			while (true) {
				// get the departure round
				if (k == (int)list.size()) {
					list.push_back(item); // insert into the end
					pifoEnque = pifoEnque + 1;
					break;
				}
				GearboxPktTag tag;
				list.at(k)->GetPacket()->PeekPacketTag(tag);
				int tagValue = tag.GetDepartureRound();
				/*if (dRound < tagValue || dRound == tagValue){
					if (dRound == tagValue){
						list.insert(list.begin() + k + 1, item); //insert into the position next to the item with the same tag
					}	
					else{
						list.insert(list.begin() + k, item); //insert into the index of k
					}
					pifoEnque = pifoEnque + 1;
					break;
				} */
				if (dRound < tagValue){
					list.insert(list.begin() + k, item); //insert into the index of k
					pifoEnque = pifoEnque + 1;
					break;
				} 
				k++;
			}
			// return the last item if pifo's size exceeds H_value, else return 0		
			if (int(list.size()) > this->H_value) {
				re = list.at(list.size() - 1);
				list.pop_back();
				pifoDeque = pifoDeque + 1;
				
				GearboxPktTag tag1;
				re->GetPacket()->PeekPacketTag(tag1);
				//tag1.SetPifodeque(tag1.GetPifodeque() + 1);
				//re->GetPacket()->ReplacePacketTag(tag1);
			}
		}
		
		if (isUpdateMaxValid == 1){
			UpdateMaxValue();
		}

		return re;
	}


	QueueDiscItem* Pifo::Pop() {
		if (list.size() == 0) {
			return NULL;
		}
		else {
			QueueDiscItem* popitem = list.front();
			list.erase(list.begin());
			if (isUpdateMaxValid == 1){
				UpdateMaxValue();
			}
			pifoDeque = pifoDeque + 1;

			GearboxPktTag tag1;
			popitem->GetPacket()->PeekPacketTag(tag1);
			//tag1.SetPifodeque(tag1.GetPifodeque() + 1);
			//popitem->GetPacket()->ReplacePacketTag(tag1);
			
			if(list.size() == 0){
				this->max_value = DEFAULT_MAX;
			}
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

	void Pifo::UpdateMaxValid(bool update){
		isUpdateMaxValid = update;
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
	
	void Pifo::SetMaxValue(int max){
		this->max_value = max;
	}

	int Pifo::GetPifoEnque(void){
		return pifoEnque;
	}

	int Pifo::GetPifoDeque(void){
		return pifoDeque;
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


	void Pifo::Print() {
		cout << "Pifo Print:";
		for (int i = 0; i < int(list.size()); i++) {
			PacketTagIterator::Item ititem = list.at(i)->GetPacket()->GetPacketTagIterator().Next();
			Callback<ObjectBase*> constructor = ititem.GetTypeId().GetConstructor();
			ObjectBase* instance = constructor();
			GearboxPktTag* tag = dynamic_cast<GearboxPktTag*> (instance);
			ititem.GetTag(*tag);
			cout << "(" << i << ": " << tag->GetDepartureRound() << ", " << tag->GetUid() << ") ";
		} 
		cout << endl;
	}

	void Pifo::PktPrint() {
		for (int i = 0; i < int(list.size()); i++) {
			PacketTagIterator::Item ititem = list.at(i)->GetPacket()->GetPacketTagIterator().Next();
			Callback<ObjectBase*> constructor = ititem.GetTypeId().GetConstructor();
			ObjectBase* instance = constructor();
			GearboxPktTag* tag = dynamic_cast<GearboxPktTag*> (instance);
			ititem.GetTag(*tag);
			if (tag->GetDepartureRound() == 3998 && tag->GetUid() == 3664){
				cout << "Pifo Print:" << "(" << i << ": " << tag->GetDepartureRound() << ", " << tag->GetUid() << ") " << endl;
			}
		} 
	}
}
