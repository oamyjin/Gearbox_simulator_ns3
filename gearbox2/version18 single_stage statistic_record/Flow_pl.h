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
    int flowId; // label
    //string flowId;
    int flowNo; // flow number
    float weight;
    //float remainingFlowSize;
    int brustness; // 07102019 Peixuan: control flow brustness level
    static const int DEFAULT_BRUSTNESS = 1000;  // 07102019 Peixuan: control flow brustness level

    int lastStartRound;
    int lastFinishRound;
    double startTime;
    double finishTime;
    
public:
    Flow_pl(int label, int no, float weight);
    Flow_pl(int label, int no, float weight, int brustness); // 07102019 Peixuan: control flow brustness level

    float getWeight() const;
    float getRemainingFlowSize() const;
    int getBrustness() const; // 07102019 Peixuan: control flow brustness level
    void setBrustness(int brustness); // 07102019 Peixuan: control flow brustness level

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
