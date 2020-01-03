
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
    virtual void sendCopyOf(myPacket *pck, int index);
    virtual int router(myPacket *pck);

  private:
    unsigned int numCx;                                     /*Define number of cx for the sender*/
    cQueue *txQueue[4];                                     /*Define queues for transmission*/
    unsigned short state_machine[4];                        /*Define state_machine to react to diff events*/
    cMessage *timeout[4];                                   /*Used as a trigger event for rtx*/
    myPacket *newPck;

};

// The module class needs to be registered with OMNeT++
Define_Module(Sender);

Sender::~Sender(){
    for (int i=0; i<numCx; ++i) {
        txQueue[i]->~cQueue();
    }
}

void Sender::initialize()
{
    numCx=getParentModule()->par("numCx");
    EV << "[" << getParentModule()->getFullName() <<"]: Sender initialized with "<<numCx<<" connections"<<endl;

    /*Initialize the Queues and the rtx timeouts for each cx*/
    for (int i=0; i<numCx-1; ++i) {
        txQueue[i] = new cQueue("txQueue");
        timeout[i] = new cMessage("timeout");
        state_machine[i] = STATE_IDLE;
        WATCH(state_machine[i]);
    }
}

void Sender::handleMessage(cMessage *msg)
{
    if (msg!=timeout[0] && msg!=timeout[1] && msg!=timeout[2] && msg!=timeout[3]) {

        /*Cast <msg> to <myPacket>*/
        myPacket *pck = check_and_cast<myPacket *>(msg);

        /*   State Machine definition:
         *     1.- First, check if the pck comes from the source
         *      1.1.- If it does and the queue is empty, send it right away
         *      1.2.- If it doesn't but the queue isn't empty, insert it
         *
         *     2.- If does not come from the source, get the index of the gate
         *         where it arrived and the index of the sending gate from router.
         *
         *     3.- Then, check the type of the pck and define 3 cases:
         *
         *      PCK: if routerIndex isn't -1 (-1 used to mark final nodes), check
         *      if the queue[routerIndex] is empty (send) or not (insert in queue)
         *
         *      ACK: check pck.destination so, if the destination is this node, the ack
         *      is coming from another node advising the pck has arrived without error and
         *      therefore this node can remove the pck from the queue[gateIndex] (and send
         *      the next queued pck if the queue isn't empty).
         *
         *      If the destination is not the node, the ack has been created by the receiver
         *      of this node (pck arrived without error), so this sender only has to send
         *      the ack to the node from which the pck came from (out[gateIndex])
         *
         *      NACK: check pck.destination so, if the destination is this node, the nack
         *      is coming from another node advising the pck has arrived with error and
         *      therefore this node has to resend the pck from the queue[gateIndex].
         *
         *      If the destination is not the node, the ack has been created by the receiver
         *      of this node (pck arrived with error), so this sender only has to send the
         *      nack to the node from which the pck came from (out[gateIndex])  */

        if (msg->arrivedOn("inS")) {

            EV << "[" << getParentModule()->getFullName() <<", sender]: New pck from source"<<endl;

            int routerIndex = router(pck);

            switch (state_machine[routerIndex]) {
                case STATE_IDLE:
                    EV << "[" << getParentModule()->getFullName() <<", sender]: Empty queue at "<<routerIndex<<". Sending"<<endl;
                    sendCopyOf(pck, routerIndex);
                    break;

                case STATE_BUSY:
                    EV << "[" << getParentModule()->getFullName() <<", sender]: Queue +1"<<endl;
                    txQueue[routerIndex]->insert(pck);
                    break;
            }
        } else {

            int gateIndex = pck->getArrivalGate()->getIndex();
            int routerIndex = router(pck);

            switch (pck->getType()) {
                case TYPE_PCK:
                    if (routerIndex == -1) {
                        EV << "[" << getParentModule()->getFullName() <<", sender]: pck reached it's destination"<<endl;
                    } else {
                        switch (state_machine[routerIndex]) {
                            case STATE_IDLE:
                                EV << "[" << getParentModule()->getFullName() <<", sender]: Empty queue ["<<routerIndex<<"]. Sending"<<endl;
                                sendCopyOf(pck, routerIndex);
                                break;

                            case STATE_BUSY:
                                EV << "[" << getParentModule()->getFullName() <<", sender]: Queue +1"<<endl;
                                txQueue[routerIndex]->insert(pck);
                                break;
                        }
                    }
                    break;

                case TYPE_ACK:
                    /*Already acknowledged, delete it and go for the next one*/
                    if (pck->getDestination()==(int)getParentModule()->par("nodeId")) {

                        /*Cancel retransmission timer*/
                        cancelEvent(timeout[gateIndex]);

                        newPck = (myPacket *)txQueue[gateIndex]->pop();

                        /*Check if the queue is empty or not*/
                        if(txQueue[gateIndex]-> isEmpty()){
                            state_machine[gateIndex] = STATE_IDLE;
                        }else{
                            /*Read first packet of the queue*/
                            newPck = (myPacket *)txQueue[gateIndex]->pop();
                            sendCopyOf(newPck,gateIndex);
                        }
                    } else {
                        send(pck,"out",gateIndex);
                    }
                    break;

                case TYPE_NACK:
                    if (pck->getDestination()==(int)getParentModule()->par("nodeId")) {

                        /*Cancel retransmission timer*/
                        cancelEvent(timeout[gateIndex]);

                        newPck = (myPacket *)txQueue[gateIndex]->pop();
                        sendCopyOf(newPck,gateIndex);

                    } else {
                        send(pck,"out",gateIndex);
                    }
                    break;
            }
        }

    } else {
        EV << "[" << getParentModule()->getFullName() <<", sender]: Timeout reached, generating new pck"<<endl;

        if (msg==timeout[0]) {
            newPck = (myPacket *)txQueue[0]->pop();
            sendCopyOf(newPck,0);
        }
        if (msg==timeout[1]) {
            newPck = (myPacket *)txQueue[1]->pop();
            sendCopyOf(newPck,1);
        }
        if (msg==timeout[2]) {
            newPck = (myPacket *)txQueue[2]->pop();
            sendCopyOf(newPck,2);
        }
        if (msg==timeout[3]) {
            newPck = (myPacket *)txQueue[3]->pop();
            sendCopyOf(newPck,3);
        }
    }
}

void Sender::sendCopyOf(myPacket *pck, int index)
{
    EV << "[" << getParentModule()->getFullName() <<", sender]: Sending pck via out["<<index<<"]"<<endl;

    /*Duplicate the pck, send one and store the copy in queue (first element) */
    myPacket *copy = pck->dup();

    if (txQueue[index]->isEmpty()) {
        txQueue[index]->insert(copy);
    } else {
        txQueue[index]->insertBefore(txQueue[index]->front(), copy);
    }

    send(pck,"out",index);
    state_machine[index]=STATE_BUSY;

    /*Set the retransmission timer to 3 times the sending time (just need to be greater than RTT)*/
    //simtime_t FinishTime = gate("out",index)->getTransmissionChannel()->getTransmissionFinishTime();
    simtime_t FinishTime = getParentModule()->gate("out",index)->getTransmissionChannel()->getTransmissionFinishTime();
    //simtime_t nextTime = simTime()+3*(FinishTime-simTime());
    simtime_t nextTime = simTime()+300*(FinishTime-simTime());
    scheduleAt(nextTime,timeout[index]);
}

int Sender::router(myPacket *pck){

    /* This function acts as a router for every node
     *
     * Node 1
     *  in/out[0] --> Node 2
     *  in/out[1] --> Node 5
     *
     * Node 2
     *  in/out[0] --> Node 1
     *  in/out[1] --> Node 3
     *  in/out[2] --> Node 4
     *  in/out[3] --> Node 5
     *
     * Node 3
     *  in/out[0] --> Node 2
     *
     * Node 4
     *  in/out[0] --> Node 2
     *  in/out[1] --> Node 5
     *
     * Node 5
     *  in/out[0] --> Node 1
     *  in/out[1] --> Node 2
     *  in/out[2] --> Node 4 */

    int arrivedNode=getParentModule()->par("nodeId");
    int destination;

    switch (arrivedNode) {
        case 1:
            destination = (rand() % 2);
            break;
        case 2:
            //destination = (rand() % 2) + 1;
            if (pck->getDestination()==3) {
                destination = 1;
            }
            if (pck->getDestination()==4) {
                destination = 2;
            }
            break;
        case 3:
            destination = -1;
            break;
        case 4:
            destination = -1;
            break;
        case 5:
            //destination = (rand() % 2) + 1;
            if (pck->getDestination()==3) {
                destination = 1;
            }
            if (pck->getDestination()==4) {
                destination = 2;
            }
            break;
    }

    return destination;
}
