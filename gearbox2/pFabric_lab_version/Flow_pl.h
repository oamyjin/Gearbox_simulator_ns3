//
// Created by Zhou Yitao on 2018-12-04.
//

#ifndef HIERARCHICALSCHEDULING_TASK_H
#define HIERARCHICALSCHEDULING_TASK_H

// will be used in package-send function
#include "ns3/queue.h"
#include "ns3/address.h"
using namespace std;

class Flow_pl {
private:
    string flowId; // label
    //string flowId;
    int flowNo; // flow number
    float weight;
    int remainingFlowSize;
    int burstness; // 07102019 Peixuan: control flow burstness level
    static const int DEFAULT_BURSTNESS = 1000;  // 07102019 Peixuan: control flow burstness level

    int lastStartRound;
    int lastFinishRound;
    double startTime;
    double finishTime;
    
public:
    Flow_pl(string label, int no, float weight, int flowsize);
    Flow_pl(string label, int no, float weight, int burstness, int flowsize); // 07102019 Peixuan: control flow burstness level
    void setRemaningSize(int remainingsize);
    int getRemaningSize();

    float getWeight() const;
    float getRemainingFlowSize() const;
    int getBurstness() const; // 07102019 Peixuan: control flow burstness level
    void setBurstness(int burstness); // 07102019 Peixuan: control flow burstness level

    int getLastStartRound() const;
    int getLastFinishRound() const;
    double getStartTime() const;
    double getFinishTime() const;
    
    void setLastStartRound(int lastStartRound);
    void setLastFinishRound(int lastFinishRound);
    //void setRemainingFlowSize(int remainingFlowSize);
    void setWeight(float weight);
    void setStartTime(double startTime);
    void setFinishTime(double startTime); 

    void setFlowNo(int no);
    int getFlowNo();

};


#endif //HIERARCHICALSCHEDULING_TASK_H
