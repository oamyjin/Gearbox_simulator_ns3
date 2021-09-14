
#include "Flow_pl.h"
#include "ns3/log.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Flow_pl");
NS_OBJECT_ENSURE_REGISTERED (Flow_pl);

TypeId Flow_pl::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Flow_pl")
    .SetParent<Object> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<Flow_pl> ()
  ;
  return tid;
}

Flow_pl::Flow_pl() : Flow_pl(DEFAULT_FLOW_ID, DEFAULT_WEIGHT) {
	NS_LOG_FUNCTION (this);
}

Flow_pl::Flow_pl(int id, float weight) {
    this->flowId = id;
    this->weight = weight;
    this->brustness = DEFAULT_BRUSTNESS;
    this->insertLevel = 0;
    this->lastDepartureRound = 0;
}

Flow_pl::Flow_pl(int id, float weight, int brustness) {
    this->flowId = id;
    this->weight = weight;
    this->brustness = brustness;
    this->insertLevel = 0;
    this->lastDepartureRound = 0;
}

Flow_pl::~Flow_pl(){
    NS_LOG_FUNCTION (this);
}

int Flow_pl::getLastDepartureRound() const {
    return lastDepartureRound;
}

void Flow_pl::setLastDepartureRound(int lastDepartureRound) {
    Flow_pl::lastDepartureRound = lastDepartureRound;
}

float Flow_pl::getWeight() const {
    return weight;
}

void Flow_pl::setWeight(float weight) {
    Flow_pl::weight = weight;
}

int Flow_pl::getInsertLevel() const {
    return insertLevel;
}

void Flow_pl::setInsertLevel(int insertLevel) {
    Flow_pl::insertLevel = insertLevel;
}

int Flow_pl::getBrustness() const {
    return brustness;
} // 07102019 Peixuan: control flow brustness level


void Flow_pl::setBrustness(int brustness) {
    Flow_pl::brustness = brustness;
} // 07102019 Peixuan: control flow brustness level

}
