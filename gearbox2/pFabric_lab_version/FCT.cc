#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <map>
using namespace std;
int main(){
	ifstream flowf;
	flowf.open("GBResult_pFabric/FCT.txt");
	string path = "GBResult_pFabric/FCT_plot.txt";
	FILE *fp;
	fp = fopen(path.data(), "a+"); //open and write
	double flowFCT[10000];
	/*
	string pkts_size;
	ifstream pktsf;
	pktsf.open("scratch/packets.txt");
	if (getline (pktsf, pkts_size)) // line中不包括每行的换行符
	{ 
			//cout << pkts_size << endl;
			char input[100000];
			cout<<(int)pkts_size.length()<<endl;
			for(int i = 0; i < (int)pkts_size.length(); i++){
				input[i] = pkts_size[i];
			}
			input[pkts_size.length()] = '\0';
			const char *d = " ";
			char *p;
			cout<<input<<endl;
			p = strtok(input,d);
			while(p)
			{
			       
				cout<<atoi(p)<<endl;
				p=strtok(NULL,d);
			}
			
	}
	if (getline (pktsf, pkts_size)) // line中不包括每行的换行符
	{ 
			//cout << pkts_size << endl;
			char input[100000];
			cout<<(int)pkts_size.length()<<endl;
			for(int i = 0; i < (int)pkts_size.length(); i++){
				input[i] = pkts_size[i];
			}
			input[pkts_size.length()] = '\0';
			const char *d = " ";
			char *p;
			cout<<input<<endl;
			p = strtok(input,d);
			while(p)
			{
			       
				cout<<atoi(p)<<endl;
				p=strtok(NULL,d);
			}
			
	}*/
	
	
	//find the last line of each flow
	int index, pre_index = 1;
	double FCT, pre_FCT;
	while (!flowf.eof()){

		flowf >> index >> FCT; 
		if(index != pre_index){
			//correspond the flow id and FCT
			flowFCT[pre_index] = pre_FCT;
			cout<<pre_index<<" "<<pre_FCT<<endl;
		}
	pre_index = index;
	pre_FCT = FCT;
	}
	if(flowf.eof()){
		flowFCT[pre_index] = pre_FCT;
		cout<<pre_index<<" "<<pre_FCT<<endl;
		
	}
	

	//store the FCT in the order of flowsize
	int flownumber, flow_no;
	int flowsorting[10000];//record the size of each flow
	ifstream infile1;
	infile1.open("scratch/traffic_pFabric.txt");
	infile1 >> flownumber;
	typedef map<uint32_t, int> FlowMap;
	FlowMap flowMap;

	for (int j = 0; j < flownumber; j++){
		uint32_t src, dst, maxPacketCount, dport;
		//double start_time,stop_time;
		double start_time;		
		//infile1 >> src >> dst >> dport >> maxPacketCount >> start_time>>stop_time;
		infile1 >> src >> dst >> dport >> maxPacketCount >> start_time;
		flowsorting[j] = maxPacketCount;
		//correspond the packet size and flow id
		flowMap[maxPacketCount] = j;
		
		
	}
	sort(flowsorting,flowsorting+flownumber);
	
	//print the list of flow size
	string path1 = "GBResult_pFabric/flow_size.txt";
	FILE *fp1;
	fp1 = fopen(path1.data(), "a+"); //open and write
	for (int p = 0; p < flownumber; p++){
		//find the corresponding flow number and get the FCT
		fprintf(fp1, "%d\t", flowsorting[p]);//flowsorting[k]
		fprintf(fp1, "\n");
	}
	fclose(fp1);
	
	
	double FCT_arr[12];
	//double FCT_base_arr[11] = {0.000013, 0.000112, 0.000143, 0.000160, 0.000184, 0.000220, 0.000460, 0.001365, 0.002558, 0.006137, 0.012102};
	double FCT_base_arr[11] = {0.000064, 0.000075, 0.000095, 0.000112, 0.000136, 0.000172, 0.000424, 0.001378, 0.002571, 0.006150, 0.012115};
	double flow_count[12];
	int interval_arr[12]= {1, 10, 20, 30, 50, 80, 200, 1000, 2000, 5000, 10000, 30000};
	//interval_arr 
	for (int k = 0; k < flownumber; k++){
		//find the corresponding flow number and get the FCT
		flow_no = flowMap[flowsorting[k]]+1;
		//FCT = flowFCT[flow_no];
			
		if(flowFCT[flow_no] == 0){
			cout<<flowFCT[flow_no] <<endl;
			cout<<"error"<<flow_no<<endl;
		}
		/*
		fprintf(fp, "%d\t", k);//flowsorting[k]
		fprintf(fp, "%f", flowFCT[flow_no]);
		fprintf(fp, "\n");
		*/
		for(int t = 0; t < 11; t++){
			if(interval_arr[t]<=flowsorting[k] && flowsorting[k]<interval_arr[t+1]){
				FCT_arr[t] += flowFCT[flow_no];
				flow_count[t] += 1; 
			}
		}
		
	}
	///*
	for(int t = 0; t < 11; t++){
		FCT_arr[t] = FCT_arr[t]/flow_count[t];
		FCT_arr[t] = FCT_arr[t]/FCT_base_arr[t];
		fprintf(fp, "%d\t", t);//flowsorting[k]
		fprintf(fp, "%f", FCT_arr[t]);
		fprintf(fp, "\n");
	}
	//*/
	




	infile1.close();
	flowf.close();
	fclose(fp);
	return 0;
}
