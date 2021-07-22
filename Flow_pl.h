//
// Created by Zhou Yitao on 2018-12-04.
//

#ifndef HIERARCHICALSCHEDULING_TASK_H
#define HIERARCHICALSCHEDULING_TASK_H

// will be used in package-send function
#include "ns3/queue.h"
#include "ns3/address.h"
using namespace std;
namespace ns3 {
class Flow_pl : public Object {
private:
    int flowId;
    //string flowId;
    float weight;
    int brustness; // 07102019 Peixuan: control flow brustness level
    static const int DEFAULT_BRUSTNESS = 1000;  // 07102019 Peixuan: control flow brustness level
    static const int DEFAULT_FLOW_ID = 1;
    static const int DEFAULT_WEIGHT = 2.0; 

    int lastDepartureRound;
    int insertLevel;
public:
    static TypeId GetTypeId (void);
    Flow_pl();
    explicit Flow_pl(int id, float weight);
    explicit Flow_pl(int id, float weight, int brustness); // 07102019 Peixuan: control flow brustness level
    ~Flow_pl();

    float getWeight() const;
    int getBrustness() const; // 07102019 Peixuan: control flow brustness level
    void setBrustness(int brustness); // 07102019 Peixuan: control flow brustness level

    int getLastDepartureRound() const;
    void setLastDepartureRound(int lastDepartureRound);

    void setWeight(float weight);

    int getInsertLevel() const;

    void setInsertLevel(int insertLevel);
}; 

}
#endif //HIERARCHICALSCHEDULING_TASK_H
