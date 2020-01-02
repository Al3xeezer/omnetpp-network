
#include <string.h>
#include <omnetpp.h>
#include "myPacket_m.h"  // Use "myPacket" packet structure

/*Type definitions for myPacket*/
#define TYPE_PCK 0
#define TYPE_ACK 1
#define TYPE_NACK 2

/*Cases for state_machine*/
#define STATE_IDLE 0
#define STATE_BUSY 1

using namespace omnetpp;


class Sender : public cSimpleModule
{
  public:
    virtual ~Sender();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void sendCopyOf(myPacket *pck);

  private:
    cQueue *txQueue;                    /*Define queues for transmission*/
    unsigned short state_machine;       /*Define state_machine to react to diff events*/
    cMessage *timeout;                  /*Used as a trigger event for rtx*/
    myPacket *newPck;
    unsigned int numCx;
};

// The module class needs to be registered with OMNeT++
Define_Module(Sender);

Sender::~Sender(){
    txQueue->~cQueue();
}



void Sender::initialize()
{
    /*Initialize the Queues and the rtx timeouts for each cx*/
    numCx = getParentModule()->par("numCx");

    cQueue *listQueue[numCx];

    for (i=0; i<numCx-1; ++i) {
        listQueue[i] = new cQueue("txQueue");
        timeout = new cMessage("timeout");
        state_machine = STATE_IDLE;
        WATCH(state_machine);
    }
}

void Sender::handleMessage(cMessage *msg)
{
    if (msg!=timeout) {

        /*Cast <msg> to <myPacket>*/
        myPacket *pck = check_and_cast<myPacket *>(msg);

        /*State Machine*/
        if (msg->arrivedOn("inS")) {

            EV << "New pck from source"<<endl;

            switch (state_machine) {
                case STATE_IDLE:
                    sendCopyOf(pck);
                    break;

                case STATE_BUSY:
                    EV << "Queue +1";
                    txQueue->insert(pck);
                    break;
            }
        } else {
            /*Cancel retransmission timer*/
            cancelEvent(timeout);

            /*Get the pending of acknowledge pck from queue*/
            newPck = (myPacket *)txQueue->pop();

            switch (pck->getType()) {
                case TYPE_ACK:
                    /*Already acknowledged, delete it and go for the next one*/
                    delete(newPck);

                    /*Check if the queue is empty or not*/
                    if(txQueue-> isEmpty()){
                        state_machine = STATE_IDLE;
                    }else{
                        /*Read first packet of the queue*/
                        newPck = (myPacket *)txQueue->pop();
                        sendCopyOf(newPck);
                    }
                    break;

                case TYPE_NACK:
                    /*Read first packet of the queue*/
                    sendCopyOf(newPck);
                    break;
            }
        }
        //delete(pck)
    } else {
        EV << "SENDER: Timeout reached: generating new pck";
        newPck = (myPacket *)txQueue->pop();
        sendCopyOf(newPck);

    }
}

void Sender::sendCopyOf(myPacket *pck)
{
    EV << "SENDER: Sending pck";

    /*Duplicate the pck, send one and store the copy in queue (first element) */
    myPacket *copy = pck->dup();

    if (txQueue->isEmpty()) {
        txQueue->insert(copy);
    } else {
        txQueue->insertBefore(txQueue->front(), copy);
    }

    send(pck,"out");

    state_machine=STATE_BUSY;

    /*Set the retransmission timer to 3 times the sending time (just need to be greater than RTT)*/
    simtime_t FinishTime = gate("out")->getTransmissionChannel()->getTransmissionFinishTime();
    simtime_t nextTime = simTime()+3*(FinishTime-simTime());
    scheduleAt(nextTime,timeout);
}
