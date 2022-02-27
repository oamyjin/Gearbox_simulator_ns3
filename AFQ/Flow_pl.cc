
#include "Flow_pl.h"

Flow_pl::Flow_pl(string id, int no, float weight) {
    this->flowId = id;
    this->flowNo = no;
    this->weight = weight;
    this->brustness = DEFAULT_BRUSTNESS;
    this->lastStartRound = 0;
    this->lastFinishRound = 0;
    //this->remainingFlowSize = 0;
    this->startTime = 0;
    this->finishTime = 0;
}

Flow_pl::Flow_pl(string id, int no, float weight, int brustness) {
    this->flowId = id;
    this->flowNo = no;
    this->weight = weight;
    this->brustness = brustness;
    this->lastStartRound = 0;
    this->lastFinishRound = 0;
    //this->remainingFlowSize = 0;
    this->startTime = 0;
    this->finishTime = 0;
}

void Flow_pl::setFlowNo(int no) {
   	this->flowNo = no;
}

int Flow_pl::getFlowNo() const {
    return flowNo;
}

int Flow_pl::getLastStartRound() const {
    return lastStartRound;
}

int Flow_pl::getLastFinishRound() const {
    return lastFinishRound;
}

double Flow_pl::getStartTime() const {
    return startTime;
}

double Flow_pl::getFinishTime() const {
    return finishTime;
}

void Flow_pl::setStartTime(double startTime) {
    Flow_pl::startTime = startTime;
}

void Flow_pl::setFinishTime(double finishTime) {
    Flow_pl::finishTime = finishTime;
}

void Flow_pl::setLastStartRound(int lastStartRound) {
    Flow_pl::lastStartRound = lastStartRound;
}

void Flow_pl::setLastFinishRound(int lastFinishRound) {
    Flow_pl::lastFinishRound = lastFinishRound;
}


float Flow_pl::getWeight() const {
    return weight;
}

void Flow_pl::setWeight(float weight) {
    Flow_pl::weight = weight;
}


int Flow_pl::getBrustness() const {
    return brustness;
} // 07102019 Peixuan: control flow brustness level


void Flow_pl::setBrustness(int brustness) {
    Flow_pl::brustness = brustness;
} // 07102019 Peixuan: control flow brustness level
