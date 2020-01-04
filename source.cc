
#include <string.h>
#include <omnetpp.h>
#include "myPacket_m.h"  // Use "myPacket" packet structure

/*Type definitions for myPacket*/
#define TYPE_PCK 0

using namespace omnetpp;


class Source : public cSimpleModule
{
  public:
    virtual ~Source();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual myPacket *createPck();
    virtual int getDestination();

  private:
    simtime_t meanTime;
    unsigned int seq;
    cMessage *msgEvent;
    bool hasSource;
};

// The module class needs to be registered with OMNeT++
Define_Module(Source);

/*Destructor... Used to remove "undisposed object:...basic.source.pck-x"*/
Source::~Source(){
    cancelAndDelete(msgEvent);
}

void Source::initialize()
{
    hasSource=getParentModule()->par("hasSource");

    if (hasSource) {
        /*Send packet to itself after meanTime (lambda=1/meanTime)*/
        meanTime=0.5;

        /*Initialize sequence number*/
        seq=1;

        EV << "[" << getParentModule()->getFullName() <<"]: Source initialized"<<endl;

        msgEvent = new cMessage("SourceEvent");
        scheduleAt(0+exponential(meanTime),msgEvent);
    }
}

void Source::handleMessage(cMessage *msg)
{
/*  The handleMessage() method is called whenever a message arrives
    at the module. Here, when an event is received, it sends the packet
    and creates another event in X seconds. Like a loop.

    exponential(): Generates random numbers from the exponential distribution.

    cancelAndDelete() remove "undisposed object:...basic.source.pck-x"   */

    cancelAndDelete(msgEvent);
    myPacket *pck = createPck();
    send(pck, "out");

    char namePck[15];
    sprintf(namePck,"pck-%d",seq);
    msgEvent = new cMessage(namePck);
    scheduleAt(simTime()+exponential(meanTime),msgEvent);

}

myPacket *Source::createPck() /*Returns the pointer of the created pck*/
{
    int origin = getParentModule()->par("nodeId");
    int dest = getDestination();

    char someName[30];
    sprintf(someName,"pck-%d, origin-%d, destination-%d",seq,origin,dest);
    myPacket *pck = new myPacket(someName,0);

    pck->setSeq(seq++);
    pck->setType(TYPE_PCK);
    pck->setBitLength(1024);
    pck->setOrigin(origin);
    pck->setDestination(dest);

    return pck;
}

int Source::getDestination(){

    /*This works by taking the remainder of the return value of the rand
     *function divided by two (which can be 0 or 1) and adding three
     *(to come up with either 3 or 4, the destination nodes).*/

    int dest = (rand() % 2) + 3;
    return dest;
}

