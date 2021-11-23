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

	Pifo::Pifo() : Pifo(10,3) {
		NS_LOG_FUNCTION(this);
	}

	Pifo::Pifo(int h, int l) {
		NS_LOG_FUNCTION(this);
		this->H_value = h;
		this->L_value = l;
		this->max_value = DEFAULT_MAX;
	}

	Pifo::~Pifo() {
		NS_LOG_FUNCTION(this);
	}


	QueueDiscItem* Pifo::Push(QueueDiscItem* item, int dRound) {
		// insert into the list directly if empty
		QueueDiscItem* re = NULL;
		//cout << "PIFO list.size():" << list.size();
		//cout << " h re:" << re << endl;	
		

		////cout<<"pifoenque times is "<<pifoenque;
		//item->GetPacket()->PeekPacketTag(tag);
		////cout<<"!!!!!!!!!!!!!!!"<<tag.GetPifoenque();

		if (list.size() == 0) {
			//pifoenque
			//modify the pifoenque tag
			//pifoenque+1
			GearboxPktTag tag;
        		item->GetPacket()->PeekPacketTag(tag);
			int pifoenque = tag.GetPifoenque()+1;
			tag.SetPifoenque(pifoenque);
			item->GetPacket()->ReplacePacketTag(tag);

			list.push_back(item);

			list.at(0)->GetPacket()->PeekPacketTag(tag);
			//cout<<"!!!!!!!!!!list.at(0) "<<tag.GetPifoenque()<<endl;
			GearboxPktTag tag1;
			list.at(list.size() - 1)->GetPacket()->PeekPacketTag(tag1);
			//cout<<"@@@@@@@@@@@list.size() - 1 "<<tag1.GetPifoenque()<<endl;
		}
		else {
			// find the right position
			int k = 0;
			while (true) {
				// get the departure round
				if (k == (int)list.size()) {
					//modify the pifoenque tag
					//pifoenque+1
					GearboxPktTag tag;
        				item->GetPacket()->PeekPacketTag(tag);
					int pifoenque = tag.GetPifoenque()+1;
					tag.SetPifoenque(pifoenque);
					item->GetPacket()->ReplacePacketTag(tag);
		
					list.push_back(item); // insert into the end

					GearboxPktTag tag1;
					list.at(list.size() - 1)->GetPacket()->PeekPacketTag(tag1);
					//cout<<"@@@@@@@@@@@list.size() - 1 "<<tag1.GetPifoenque()<<endl;
					break;
				}
				GearboxPktTag tag;
				list.at(k)->GetPacket()->PeekPacketTag(tag);
				int tagValue = tag.GetDepartureRound();
				if (dRound < tagValue || dRound == tagValue){
					//modify the pifoenque tag
					//pifoenque+1
					GearboxPktTag tag;
        				item->GetPacket()->PeekPacketTag(tag);
					int pifoenque = tag.GetPifoenque()+1;
					tag.SetPifoenque(pifoenque);
					item->GetPacket()->ReplacePacketTag(tag);

					list.insert(list.begin() + k, item); //insert into the index of k

					GearboxPktTag tag1;
					list.at(k)->GetPacket()->PeekPacketTag(tag1);
					//cout<<"!!!!!!!!!!list.at(k) "<<tag1.GetPifoenque()<<endl;


					break;
				} 
				k++;
			}
			// return the last item if pifo's size exceeds H_value, else return 0	
			//cout << "int(list.size()):" << int(list.size()) << " this->H_value:" << this->H_value << endl;
			//cout << "hh re:" << re << endl;	
			if (int(list.size()) > this->H_value) {
				//cout << "in" << endl;
				re = list.at(list.size() - 1);
				list.pop_back();
				//cout << "here is re" << endl;
				//modify the pifodeque tag
				//pifodeque+1
				GearboxPktTag tag;

        			re->GetPacket()->PeekPacketTag(tag);
				int pifodeque = tag.GetPifodeque()+1;

				tag.SetPifodeque(pifodeque);
				re->GetPacket()->ReplacePacketTag(tag);

				//cout<<"pifodeque times is "<<pifodeque;
			}
			

		}
		
		UpdateMaxValue();
		//cout << "PIFO end re:" << re << endl;
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
			//modify the pifodeque tag
			//pifodeque+1
			GearboxPktTag tag;

        		popitem->GetPacket()->PeekPacketTag(tag);
			int pifodeque = tag.GetPifodeque()+1;

			tag.SetPifodeque(pifodeque);
			popitem->GetPacket()->ReplacePacketTag(tag);
	
			//cout<<"@@@@@@@@@@@pifoenque "<<tag.GetPifoenque()<<endl;
			//cout<<"pifodeque times is "<<pifodeque;
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
			cout << "(" << i << ": " << tag->GetDepartureRound() << ", " << tag->GetPifoenque() <<") ";
		}
		cout << endl;
	}
