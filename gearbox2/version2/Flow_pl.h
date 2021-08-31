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
    int flowId;
    //string flowId;
    float weight;
    int brustness; // 07102019 Peixuan: control flow brustness level
    static const int DEFAULT_BRUSTNESS = 1000;  // 07102019 Peixuan: control flow brustness level

    int lastStartRound;
    int lastFinishRound;
    int insertLevel;
public:
    Flow_pl(int id, float weight);
    Flow_pl(int id, float weight, int brustness); // 07102019 Peixuan: control flow brustness level

    float getWeight() const;
    int getBrustness() const; // 07102019 Peixuan: control flow brustness level
    void setBrustness(int brustness); // 07102019 Peixuan: control flow brustness level

    int getLastStartRound() const;
    int getLastFinishRound() const;
    void setLastStartRound(int lastStartRound);
    void setLastFinishRound(int lastFinishRound);

    void setWeight(float weight);

    int getInsertLevel() const;

    void setInsertLevel(int insertLevel);
};


#endif //HIERARCHICALSCHEDULING_TASK_H
