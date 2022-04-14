#include <iostream>
#include <fstream>
using namespace std;
 
int main()
{
    int inversion=0;
    int datalen=0;
    double num[1000000];
    ifstream file("GBResult/pktsList/DequeuedPktsList.txt");
    while( ! file.eof() )
    	file>>num[datalen++];
    file.close();

    cout << "datalen:" << datalen << endl;

    int m =0;
    for(int i=0;i<datalen-1;i++)
    {
        for(int j=0; j<i; j++){
		if(num[j]>num[i]){
			inversion = inversion + num[j]-num[i];
			m++;
			if (m <= 20){
				cout<<"@"<<num[i]<<"@"<<num[j]<<endl;
			}
		}
	}
    }

    cout<<"inversion magnitude= "<<inversion<<endl;


    int inversion_count = 0;
    for(int m=0;m<datalen-1;m++){
	int min=num[m];

	for(int i=m;i<datalen-1; i++){
        	if(num[i]<min)
            	min=num[i];
	}

	if(num[m] > min){
		inversion_count += 1;	
	}

    }

    cout<<"inversion count= "<<inversion_count<<endl;
    

    return 0;
}
