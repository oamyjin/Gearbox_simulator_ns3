// 方法一
// 将源文件中的特定字符串替换,内容输出到新文件中

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include "Replace_string.h"
#include "ns3/log.h"
#include <vector>
using namespace std;


namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("Replace_string");
	NS_OBJECT_ENSURE_REGISTERED(Replace_string);

	TypeId Replace_string::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::Replace_string")
			.SetParent<Object>()
			.SetGroupName("TrafficControl")
			.AddConstructor<Replace_string>()
			;
		return tid;
	}

	Replace_string::Replace_string() {
		NS_LOG_FUNCTION(this);
	}


	Replace_string::~Replace_string() {
		NS_LOG_FUNCTION(this);
	}

	bool Replace_string::FixNewFile(QueueDiscItem* item, int uid, string target, float time)
	{	
		Packet* packet = GetPointer(item->GetPacket());
		GearboxPktTag tag;
		packet->PeekPacketTag(tag);
		//cout<<"flow number"<<tag.GetFlowNo()<<endl;
		//int flowNo = tag.GetFlowNo();
		fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord.txt"); 
		//fstream fixstream;
		/*if(flowNo == 1){
			fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord1.txt"); 
		}
		else if(flowNo == 2){
			fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord2.txt"); 
		}
		else if(flowNo == 3){
			fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord3.txt"); 
		}
		else{cout<<"error"<<endl;}*/
		// fstream 默认以读写方式打开文件,支持对同一文件同时读写操作
		if(!fixstream){  
  			cout<<"不能打开目的文件：test.txt"<<'\n';  
  			exit(1);  
 		}

		string str;
		size_t cur; // 记录读指针位置
		size_t pos,pos0 = 0;
		while (getline(fixstream, str))
		{	//cout<<str<<endl;
			pos0 = str.find("uid:"+std::to_string(uid));
			if (pos0 != string::npos)
			{	
				int pointer = -1;
				if( target=="fifoenque"){
					pos = str.find("fifoenque:");
					pointer = 0;
					//cout<<"fifoenque"<<endl;
				}
				else if(target=="fifodeque"){
					pos = str.find("fifodeque:");
					pointer = 1;
				}
				else if(target=="pifoenque"){
					pos = str.find("pifoenque:");
					pointer = 2;
				}
				else if(target=="pifodeque"){
					pos = str.find("pifodeque:");
					pointer = 3;
				}
				else if(target=="overflow"){
					pos = str.find("overflow:");
					pointer = 4;
				}
				else if(target=="reload"){
					pos = str.find("reload:");
					pointer = 5;
				}
				else if(target=="migration"){
					pos = str.find("migration:");
					pointer = 6;
				}
				else if(target=="enquetime"){
					pos = str.find("enquetime:");
					pointer = 7;
				}
				else if(target=="dequetime"){
					pos = str.find("dequetime:");
					pointer = 8;
				}
				else {cout<<"error"<<endl;}
				cur = fixstream.tellg();
				size_t len = strlen(str.c_str()) +1;
				int* output = GetNumber(str);
				//cout<<"output2"<<output[1]<<endl;
				fixstream.seekp(-1*len, fstream::cur); // (读写指针本来在相同位置),此时写指针回退到上一行
				//cout<<"ouput[pointer]"<<output[pointer];
				if(pointer<=6){
					if((output[pointer]+1) >10 ){
						str.replace(pos+strlen(target.c_str()) +1, 2, std::to_string(output[pointer]+1));
					}
					else{
						str.replace(pos+strlen(target.c_str()) +1, 1, std::to_string(output[pointer]+1));
					}
					if(uid == 4437){
						//cout<<"@@"<<std::to_string(output[pointer])<<endl;
					}
					
				}
				else{
					str.replace(pos+strlen(target.c_str()) +1, 8, std::to_string(time));
				}
				
				//len = strlen(str.c_str()) ;
				//cout<<len<<endl;
				//fixstream.clear();
				fixstream << str<< endl;
				if(uid == 4437){
						//cout<<str<<endl;
				}
				//cout<<str<<endl;
				fixstream.seekp(cur); // 写指针位置还原
				continue;
			}
			
		}
		fixstream.close();
		return true;
	
	}

	int* Replace_string::GetNumber(string str){
		char input[1000];
		for(int k = 0; k < (int)str.length(); k++){
	    		input[k] = str[k];
		}
		input[str.length()] = '\0';

		char* token = std::strtok(input, ": ");
		int* output = new int[7];
		int i=0;
		int j=0;
		    while (token != NULL) {
			//cout<<"loop"<<endl;
			i++;
			//cout<<"i"<<i<<endl;
			if((i==9)|(i==11)|(i==13)|(i==15)|(i==17)|(i==19)|(i==21)){
				output[j]= atoi(token);
				//std::cout << "output"<<output[j] << '\n';
				//std::cout <<  "token"<<token << '\n';
				j++;

			
			}
			
			//std::cout << token << '\n';
			token = std::strtok(NULL, ": ");
		    }
		return output;

	}

	/*int Replace_string::metadata_access_time(){
		if(flowNo = 1){
			fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord1.txt"); 
		}
		else if(flowNo = 2){
			fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord2.txt"); 
		}
		else if(flowNo = 3){
			fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord3.txt"); 
		}
		else{cout<<"error"<<endl;}
		// fstream 默认以读写方式打开文件,支持对同一文件同时读写操作
		if(!fixstream){  
  			cout<<"不能打开目的文件：test.txt"<<'\n';  
  			exit(1);  
 		}
		int statis = 0;
		string str;
		while (getline(fixstream, str)){
			int* output = GetNumber(str);
			for(int i=0;i<7;i++){
				statis += output[i];
			}
		}
		return statis;
	}*/



}
