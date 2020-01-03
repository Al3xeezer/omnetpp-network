
#include <string.h>
#include <omnetpp.h>
#include "myPacket_m.h"  // Use "myPacket" packet structure

/*Type definitions for myPacket*/
#define TYPE_PCK 0
#define TYPE_ACK 1
#define TYPE_NACK 2


using namespace omnetpp;


class Receiver : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void createPck(unsigned int seq, unsigned short TYPE, int index, int dest);
};

// The module class needs to be registered with OMNeT++
Define_Module(Receiver);


void Receiver::initialize()
{

}

void Receiver::handleMessage(cMessage *msg)
{
    EV << "[" << getParentModule()->getFullName() <<", receiver]: New pck in receiver"<<endl;
    myPacket *pck = check_and_cast<myPacket *>(msg);

    /*The sending gate has the same index as the incoming one*/
    int gate = pck->getArrivalGate()->getIndex();

    switch (pck->getType()) {
        case TYPE_PCK:
            if(pck->hasBitError()){
                EV << "[" << getParentModule()->getFullName() <<", receiver]: Has error. Sending NACK to origin"<<endl;
                createPck(pck->getSeq(), TYPE_NACK, gate, pck->getSenderModule()->getParentModule()->par("nodeId"));
            }
            else
            {
                EV << "[" << getParentModule()->getFullName() <<", receiver]: No error. Sending ACK to origin"<<endl;
                createPck(pck->getSeq(), TYPE_ACK, gate, pck->getSenderModule()->getParentModule()->par("nodeId"));
                send(pck,"out",gate);
            }
            break;
        case TYPE_ACK:
            EV << "[" << getParentModule()->getFullName() <<", receiver]: ACK received at in["<<gate<<"]"<<endl;
            send(pck,"out",gate);
            break;
        case TYPE_NACK:
            EV << "[" << getParentModule()->getFullName() <<", receiver]: NACK received at in["<<gate<<"]"<<endl;
            send(pck,"out",gate);
            break;
    }

}

/*Returns the pointer of the created ack/nack packet*/
void Receiver::createPck(unsigned int seq, unsigned short TYPE, int index, int dest)
{
    int origin = getParentModule()->par("nodeId");

    char someName[30];
    if (TYPE == TYPE_ACK) {
        sprintf(someName,"ack-%d, receiver-%d",seq,origin);
    } else {
        sprintf(someName,"nack-%d, receiver-%d",seq,origin);
    }

    myPacket *pck = new myPacket(someName,0);

    pck->setSeq(seq);
    pck->setType(TYPE);
    pck->setBitLength(1024);
    pck->setOrigin(origin);
    pck->setDestination(dest);

    send(pck,"out",index);
}

