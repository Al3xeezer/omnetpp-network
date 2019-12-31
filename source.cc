
#include <string.h>
#include <omnetpp.h>
#include "myPacket_m.h"  // Use "myPacket" packet structure

/*Type definitions for myPacket*/
#define TYPE_PCK 0
#define TYPE_ACK 1
#define TYPE_NACK 2

using namespace omnetpp;


class source : public cSimpleModule
{
  public:
    virtual ~source();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual myPacket *createPck();

  private:
    simtime_t meanTime;
    unsigned int seq;
    cMessage *msgEvent;
};

// The module class needs to be registered with OMNeT++
Define_Module(source);

/*Destructor... Used to remove "undisposed object:...basic.source.pck-x"*/
source::~source(){
    cancelAndDelete(msgEvent);
}


void source::initialize()
{
    /*Send packet to itself after meanTime (lambda=1/meanTime)*/
    meanTime=0.5;

    /*Initialize sequence number*/
    seq=1;

    msgEvent = new cMessage("INIT SOURCE");
    scheduleAt(meanTime,msgEvent);

}

void source::handleMessage(cMessage *msg)
{
/*   The handleMessage() method is called whenever a message arrives
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

myPacket *source::createPck() /*Returns the pointer of the created pck*/
{
    char someName[15];
    sprintf(someName,"pck-%d",seq);
    myPacket *pck = new myPacket(someName,0);

    pck->setSeq(seq++);
    pck->setType(TYPE_PCK);
    pck->setBitLength(1024);

    return pck;
}

