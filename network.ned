//
// Simple modules: sender, receiver, source. These modules define the 
// different functionalities that a node need to implement in order to
// be capable of transmitting packets to another node
//

simple Sender
{
    @display("i=block/tx");
    gates:
        input in[];
        output out[];
        input inS;
}

simple Receiver
{
    @display("i=block/rx");
    gates:
        input in[];
        output out[];
}

simple Source
{
    @display("i=block/source");
    gates:
        output out;
}

//
// Compound module: node. This module gather the different simple modules 
// previously defined.
//
module Node
{
    parameters:
        @display("bgb=306,183;i=block/routing");
        bool hasSource = default(false);
        int numCx = default(4);
        int nodeId;
    gates:
        input in[];
        output out[];
    submodules:
        receiver: Receiver {
            @display("p=194.48001,114.07");
        }
        sender: Sender {
            @display("p=126,51");
        }
        source: Source {
            @display("p=34,51");
        }
    connections:
        for i=0..sizeof(in)-1 {
            sender.out++ --> out++;
            receiver.in++ <-- in++;
            receiver.out++ --> sender.in++;
        }
        source.out --> sender.inS;

}

//
// Network definition 
//
network Complete
{
    @display("bgb=428,222");
    types:
        channel Ethernet100 extends ned.DatarateChannel
        {
            //datarate = 100Mbps;
            //datarate = 9600bps;
            datarate = 0.1Mbps;
            delay=10ms;
            per = 0.5;
        }
    submodules:
        node1: Node {
            @display("p=58,105");
        }
        node2: Node {
            @display("p=184,50");
        }
        node3: Node {
            @display("p=344,50");
        }
        node4: Node {
            @display("p=344,168");
        }
        node5: Node {
            @display("p=161,168");
        }
    connections:
        // Connection NODE1 <-> NODE2
        node1.out++ --> Ethernet100 --> node2.in++;
        node1.in++ <-- Ethernet100 <-- node2.out++;
        // Connection NODE1 <-> NODE5
        node1.out++ --> Ethernet100 --> node5.in++;
        node1.in++ <-- Ethernet100 <-- node5.out++;
        // Connection NODE2 <-> NODE3
        node2.out++ --> Ethernet100 --> node3.in++;
        node2.in++ <-- Ethernet100 <-- node3.out++;
        // Connection NODE2 <-> NODE4
        node2.out++ --> Ethernet100 --> node4.in++;
        node2.in++ <-- Ethernet100 <-- node4.out++;
        // Connection NODE2 <-> NODE5
        node2.out++ --> Ethernet100 --> node5.in++;
        node2.in++ <-- Ethernet100 <-- node5.out++;
        // Connection NODE4 <-> NODE5
        node4.out++ --> Ethernet100 --> node5.in++;
        node4.in++ <-- Ethernet100 <-- node5.out++;
}
