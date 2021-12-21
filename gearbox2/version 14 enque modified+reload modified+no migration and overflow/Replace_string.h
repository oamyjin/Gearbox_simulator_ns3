#ifndef REPLACE_STRING_H
#define REPLACE_STRING_H
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include "ns3/object.h"
#include "ns3/type-id.h"
#include <vector>
#include "ns3/gearbox-pkt-tag.h"
#include "ns3/queue-disc.h"
using namespace std;

namespace ns3 {
    class Replace_string:public Object {
	public:
		Replace_string();
		~Replace_string();
		static TypeId GetTypeId(void);

		bool FixNewFile(QueueDiscItem* item, int uid, string target, float time);
		int* GetNumber(string str);
		//int metadata_access_time();


   };
		
}
#endif //REPLACE_H
