/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ==== Node0 <-> Node1 <-> Node1
 * ==== Gearbox on the link from middleNode to Node1
 *
 *
 *
   A(1)   --------
                 |  
   A(2)   -----\ |
                \| 
   ...    ------ router(0) ------- S(128)
                /|
   A(126) -----/ |  
                 |
   A(127) --------
 */

#include <iostream>
#include <fstream>
#include "ns3/udp-header.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/stats-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-header.h"
#include <map> 


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("MyTopoUDP");

const int CLIENT_CNT = 20; // number of client hosts
const int SERVER_CNT = 1; // there is only 1 server host
const int ROUTER_CNT = 1; // there is only 1 router

uint32_t previous[CLIENT_CNT] = {0};
uint32_t previoustx[CLIENT_CNT] = {0};
Time prevTime[CLIENT_CNT] = {Seconds(0)};

double nSamplingPeriod = 0.1;
double simulator_stop_time = 20.01;




// maintain port number for each host pair (src, port), and the dst is awalys the last host
std::map<uint32_t, uint16_t> portNumder;
// ip address pairs on the same links
std::vector<Ipv4InterfaceContainer> interfaces(CLIENT_CNT + SERVER_CNT);
// Create nodes: client + router + server = nodes
NodeContainer clientNodes, routers, serverNode, nodes;

ifstream flowf;

struct FlowInput{
	uint32_t src, dst, maxPacketCount, port, dport;
	double start_time;
	uint32_t idx;
};
FlowInput flow_input = {0};
uint32_t flow_num;

void ReadFlowInput(){
	if (flow_input.idx < flow_num){
		flowf >> flow_input.src >> flow_input.dst >> flow_input.dport >> flow_input.maxPacketCount >> flow_input.start_time; //pg: priority group (unused)
                cout << "flow_input.start_time:" << flow_input.start_time << endl;
		NS_ASSERT(flow_input.src != 0 && flow_input.dst == CLIENT_CNT + SERVER_CNT);
	}
}
void ScheduleFlowInputs(){
	while (flow_input.idx < flow_num && Seconds(flow_input.start_time) == Simulator::Now()){
		uint32_t port = portNumder[flow_input.src]++; // get a new port number 
                cout << "flow_input.src:" << flow_input.src << " port:" << port << endl;
		UdpEchoServerHelper echoServer (port);

                ApplicationContainer serverApps = echoServer.Install (nodes.Get (CLIENT_CNT + SERVER_CNT));
                serverApps.Start (Seconds (0.0));
                serverApps.Stop (Seconds (10.0));

                UdpEchoClientHelper echoClient (interfaces[CLIENT_CNT + SERVER_CNT - 1].GetAddress (1), port);
                echoClient.SetAttribute ("MaxPackets", UintegerValue (flow_input.maxPacketCount));
                echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.00008))); //0.00008s=>100Mbps, 0.002s=>4Mbps
                echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

                ApplicationContainer clientApps = echoClient.Install (nodes.Get (flow_input.src));
                clientApps.Start (Seconds (0.0));

		// get the next flow input
		flow_input.idx++;
		ReadFlowInput();
	}

	// schedule the next time to run this function
	if (flow_input.idx < flow_num){
                cout << "flow_input.idx:" << flow_input.idx << " Seconds(flow_input.start_time):" << Seconds(flow_input.start_time) << " Simulator::Now():" << Simulator::Now() << " Seconds(flow_input.start_time)-Simulator::Now():" << Seconds(flow_input.start_time)-Simulator::Now() << endl;
		Simulator::Schedule(Seconds(flow_input.start_time)-Simulator::Now(), ScheduleFlowInputs);
	}else { // no more flows, close the file
		flowf.close();
	}
}



static void
ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> monitor)
{
  //FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  double localThrou = 0.0;
  //double localThroutx = 0.0;
  monitor->CheckForLostPackets ();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
 	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	Time curTime = Now ();
        // record to corresponding flow
        for (int k = 0; k < CLIENT_CNT; k++){
                stringstream srcip;
                srcip << "10.1." << k + 1 << ".2";
                stringstream dstip;
                dstip << "10.1." << CLIENT_CNT + SERVER_CNT << ".2";
                cout << srcip.str() << endl;
	        if (t.sourceAddress == Ipv4Address(srcip.str().c_str()) && t.destinationAddress == Ipv4Address(dstip.str().c_str())){
                        stringstream path;
                        path << "UDPResult/" << k + 1 << "throughput.dat";
		        std::ofstream thr(path.str(), std::ios::out | std::ios::app);
                        localThrou = 8 * (i->second.rxBytes - previous[k]) / (1024 * 1024 * (curTime.GetSeconds () - prevTime[k].GetSeconds ())); 
                        //localThroutx =  8 * (i->second.txBytes - previous1tx) / (1024 * 1024 * (curTime.GetSeconds () - prevTime1.GetSeconds ()));
		        //thr <<  curTime.GetSeconds () << " Trx:" << localThrou << " rxBytes:" << i->second.rxBytes << "  Ttx:" << localThroutx << " txBytes:" << i->second.txBytes << std::endl;
                        thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		        cout << "S:" << curTime.GetSeconds() << endl;
		        prevTime[k] = curTime;
		        previous[k] = i->second.rxBytes;
                        previoustx[k] = i->second.txBytes;
		        cout << "here==========" << k+1 << "=============" << endl; 
                        break;
           	}
        }
  }
  Simulator::Schedule (Seconds(nSamplingPeriod), &ThroughputMonitor, fmhelper, monitor);
}

int
main (int argc, char *argv[])
{
        clock_t begint, endt;
	begint = clock();

        CommandLine cmd;
        cmd.Parse (argc, argv);
        Time::SetResolution (Time::NS);

        flowf.open("/home/pc/ns-allinone-3.26/ns-3.26/scratch/flow.txt");
	flowf >> flow_num;
        cout << "flow_num:" << flow_num << endl;

        // Create nodes
        routers.Create (ROUTER_CNT);// 0
        clientNodes.Create (CLIENT_CNT);// 1,2,3 ... 127
        serverNode.Create(SERVER_CNT);// 128
        nodes = NodeContainer(routers, clientNodes, serverNode);
        // Adjacent nodes pairs: client--node, node--server
        std::vector<NodeContainer> nodeAdjacencyList(CLIENT_CNT + SERVER_CNT); 
        for (int i = 0; i < CLIENT_CNT + SERVER_CNT; i++){ 
              nodeAdjacencyList[i]=NodeContainer(nodes.Get(0), nodes.Get(i+1)); // adjacent node pair k: (router(0), Clinet(k+1)), the last one is (router, server)
        }

        // Configure channels
        std::vector<PointToPointHelper> p2p(CLIENT_CNT + SERVER_CNT);
        // client -- router, router -- server
        for (int i = 0; i < CLIENT_CNT + SERVER_CNT; i++){      
                p2p[i].SetDeviceAttribute ("DataRate", StringValue ("1Gbps")); // Bandwith
                p2p[i].SetChannelAttribute ("Delay", StringValue ("1ms"));  // Dely
                p2p[i].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1)); // Buffer of transmission queue
        }
        // install configurations to devices (NIC)
        std::vector<NetDeviceContainer> devices(CLIENT_CNT + SERVER_CNT);
        for (int i = 0; i < CLIENT_CNT + SERVER_CNT; i ++){
                devices[i] = p2p[i].Install(nodeAdjacencyList[i]);
        }

        // Install network protocals
        InternetStackHelper stack;  
        stack.Install (nodes); // install tcp, udp, ip... 
 
        // Assign ip address
        Ipv4AddressHelper address;    
        for (uint32_t i = 0; i < CLIENT_CNT + SERVER_CNT; i++)  
        {  
                std::ostringstream subset;  
                subset << "10.1." << i + 1 << ".0"; 
                // n0: 10.1.1.1    n1 NIC1: 10.1.1.2   n1 NIC2: 10.1.2.1   n2 NIC1: 10.1.2.2  ...
                address.SetBase(subset.str().c_str (), "255.255.255.0"); // sebnet & mask  
                interfaces[i] = address.Assign(devices[i]); // set ip addresses to NICs: 10.1.1.1, 10.1.1.2 ...  
        }  

        // unintall traffic control
        TrafficControlHelper tch;
        for (int i = 0; i < CLIENT_CNT + SERVER_CNT; i++){
              tch.Uninstall(devices[i]);
        }

        //tch.SetRootQueueDisc ("ns3::Gearbox_pl_fid_flex");
        tch.SetRootQueueDisc ("ns3::AFQ_10_unlim_pl");
        tch.Install(devices[CLIENT_CNT + SERVER_CNT - 1]);
  
        // Create router nodes, initialize routing database and set up the routing tables in the nodes.
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

        //Time interPacketInterval = Seconds(0.0000005 / 2); //TODO: where to use

	// maintain port number for each host pair
	for (uint32_t i = 1; i <= CLIENT_CNT; i++){
		portNumder[i] = 10000; // each (host, server) pair use port number from 10000
	}

        flow_input.idx = 0;
       	if (flow_num > 0){
		ReadFlowInput();
		Simulator::Schedule(Seconds(flow_input.start_time)-Simulator::Now(), ScheduleFlowInputs);
	}

        // monitor throughput (TODO: Flow: src, dst, port)
        FlowMonitorHelper flowmon;
        Ptr<FlowMonitor> monitor = flowmon.Install(nodes);

        ThroughputMonitor (&flowmon, monitor);

        std::cout << "Running Simulation.\n";
	fflush(stdout);
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(simulator_stop_time));
        //ThroughputMonitor (&flowmon, monitor);
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");

	endt = clock();
	std::cout << (double)(endt - begint) / CLOCKS_PER_SEC << "\n";

        return 0;
}

