#include "Flow_pl.h"

Flow_pl::Flow_pl(string id, int no, float weight, int flowsize) {
    this->flowId = id;
    this->flowNo = no;
    this->weight = weight;
    this->burstness = DEFAULT_BURSTNESS;
    this->lastStartRound = 0;
    this->lastFinishRound = 0;
    this->remainingFlowSize = flowsize;
    this->startTime = 0;
    this->finishTime = 0;
}

Flow_pl::Flow_pl(string id, int no, float weight, int burstness, int flowsize) {
    this->flowId = id;
    this->flowNo = no;
    this->weight = weight;
    this->burstness = burstness;
    this->lastStartRound = 0;
    this->lastFinishRound = 0;
    this->remainingFlowSize = flowsize;
    this->startTime = 0;
    this->finishTime = 0;
}

void Flow_pl::setRemaningSize(int remainingsize){
    this->remainingFlowSize = remainingsize;
}
   
int Flow_pl::getRemaningSize(){
    return this->remainingFlowSize;
}

void Flow_pl::setFlowNo(int no){
    this->flowNo = no;
}
   
int Flow_pl::getFlowNo(){
    return this->flowNo;
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


int Flow_pl::getBurstness() const {
    return burstness;
} // 07102019 Peixuan: control flow burstness level


void Flow_pl::setBurstness(int burstness) {
    Flow_pl::burstness = burstness;
} // 07102019 Peixuan: control flow burstness level
