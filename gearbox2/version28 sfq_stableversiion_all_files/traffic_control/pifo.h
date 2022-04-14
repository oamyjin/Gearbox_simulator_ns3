/*
 * this implementation refers to PIFO Queue
 * just like a linkedlist
 *
 * Author: Jiajin
 */
#ifndef PIFO_H
#define PIFO_H

#include <vector>
#include "ns3/gearbox-pkt-tag.h"
#include "ns3/queue-disc.h"

using namespace std;

namespace ns3 {

	class Pifo : public Object {

	public:

		/**
		 *
		 * Copy constructor, copies the data and increases reference count
		 *
		 * \param int: H_value, int: L_value
		 * \returns The newly created ByteTagList
		 *
		 */
		Pifo();
		Pifo(int, int);
		~Pifo();

		static TypeId GetTypeId(void);

		/**
		 * \param item: pkt to be enqued, departureRound: the pkt's tag value
		 * \returns The tail queueitems if the pifo size exceeds H_value, else 0
		 *
		 * Enque into Pifo at the right position (increasing, the top is the smallest)
		 */
		QueueDiscItem* Push(QueueDiscItem* item, int departureRound);

		/**
		* \returns The top queueitem
		*
		* Deque the pkt with the smallest departure round
		*/
		QueueDiscItem* Pop(void);
		/**
		* \returns The top queueitem
		*
		* Peek the pkt with the smallest departure round
		*/
		QueueDiscItem* Peek(void);


		/**
		 * \returns The number of items in the pifo
		 *
		 * get the size of pifo
		 */
		int Size(void);

		/**
		*\returns The max departure round tag value in pifo
		*/
		int GetMaxValue(void);
		
		/**
		*set the max tag value in pifo
		* \input: the value of the maximum tag in the pifo
		*/
		void SetMaxValue(int);

		/**
		*update the max tag value in pifo
		*/
		void UpdateMaxValue(void);

		/**
		*update the value of isUpdateMaxValid in pifo
		*1: update pifoMax automatically
		*0: do not update pifoMax
		*/
		void UpdateMaxValid(bool);

		/**
		*print the values in the pifo
		*/
		void Print();
		
		void PktPrint();

		/**
		*\returns the L_value in pifo
		*/
		int LowestSize(void);

		/**
		*\returns the H_value in pifo
		*/
		int HighestSize(void);

		/**
		*\return the number of pifoenque operations
		*/
		int GetPifoEnque(void);
		/**
		*\return the number of pifodeque operations
		*/
		int GetPifoDeque(void);


		/**
		  * \returns tag values: departureRound,  index
		*
		* Get the values of the pkt's tag:
		  */
		vector<int> GetTagValue(QueueDiscItem* item);

	private:

		int H_value;
		int L_value;
		int max_value;
		bool isUpdateMaxValid;
		// record operation count
	        int pifoEnque = 0;
	        int pifoDeque = 0;
		static const int DEFAULT_MAX = 99999999;
		vector<QueueDiscItem*> list; // pifo queue list

	};

} // namespace ns3
#endif //PIFO_H
