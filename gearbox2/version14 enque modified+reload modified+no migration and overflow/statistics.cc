#include "ns3/Replace_string.h"

using namespace std;
int main(){
	int statis = 0;
	int fifoenque =0;
	int fifodeque =0;
	int pifoenque =0;
	int pifodeque =0;
	int overflow =0;
	int reload =0;
	int migration =0;
	string str;
	//for(int i=1;i<4;i++){

		string path = "GBResult/pktsList/tagRecord";
		//path.append(to_string(i));
		path.append(".txt");
		fstream fixstream(path.data()); // fstream 默认以读写方式打开文件,支持对同一文件同时读写操作
		//fstream fixstream("/home/pc/ns-allinone-3.26/ns-3.26/GBResult/pktsList/tagRecord.txt");
		if(!fixstream){  
  			cout<<"不能打开目的文件：test.txt"<<'\n';  
  			exit(1);  
 		}
		
		
		while (getline(fixstream, str)){
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
				i++;
				if((i==9)|(i==11)|(i==13)|(i==15)|(i==17)|(i==19)|(i==21)){
					output[j]= atoi(token);
					j++;

			
				}
				token = std::strtok(NULL, ": ");
		   	 }

			for(int i=0;i<4;i++){
				statis += output[i];
			}
			fifoenque += output[0]; 
			fifodeque += output[1];
			pifoenque += output[2];
			pifodeque += output[3];
			overflow += output[4];
			reload += output[5];
			migration += output[6];
		}
		fixstream.close();

	//}
	
	cout << "statistics\n"<<statis<<"\nfifoenque\n"<<fifoenque<<"\nfifodeque\n"<<fifodeque<<"\npifoenque\n"<<pifoenque<<"\npifodeque\n"<<pifodeque<<"\noverflow\n"<<overflow<<"\nreload\n"<<reload<<"\nmigration\n"<<migration<<endl;

}

