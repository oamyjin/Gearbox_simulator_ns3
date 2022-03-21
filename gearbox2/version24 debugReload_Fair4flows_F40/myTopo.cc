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

NS_LOG_COMPONENT_DEFINE ("MyTopoTCP");

const int CLIENT_CNT = 4; // number of client hosts
const int SERVER_CNT = 1; // there is only 1 server host
const int ROUTER_CNT = 1; // there is only 1 router

const char* DATARATE = "10Gbps"; // Bandwith 10G
const char* DELAY = "3us"; // Delay 3us (0.000001s -> 0.0001s)  (3ms = 0.001s -> 0.1s)

uint32_t previous[CLIENT_CNT<<16] = {0};
uint32_t previoustx[CLIENT_CNT<<16] = {0};
Time prevTime[CLIENT_CNT<<16] = {Seconds(0)};


double nSamplingPeriod = 0.0001; // 0.000006115;
double simulator_stop_time = 0.0301;


// maintain port number for each host pair (src, portArray), and the dst is awalys the last host
std::map<uint32_t, int> portMap;
// ip address pairs on the same links
std::vector<Ipv4InterfaceContainer> interfaces(CLIENT_CNT + SERVER_CNT);
// Create nodes: client + router + server = nodes
NodeContainer clientNodes, routers, serverNode, nodes;
// Help to monitor cwnd
AsciiTraceHelper asciiTraceHelper;



class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{   
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);  

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

/*
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  //stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << Simulator::Now ().GetSeconds () - prevTime1.GetSeconds () << "\t" << newCwnd << std::endl;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;

}



static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}
*/


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
                cout << "flow_input.src:" << flow_input.src << " flow_input.dst:" << flow_input.dst << " CLIENT_CNT + SERVER_CNT:" << CLIENT_CNT + SERVER_CNT << endl;
		NS_ASSERT(flow_input.src != 0 && flow_input.dst == CLIENT_CNT + SERVER_CNT);
	}
}
void ScheduleFlowInputs(){
	while (flow_input.idx < flow_num && Seconds(flow_input.start_time) == Simulator::Now()){
                int port = portMap[flow_input.src] + 1; // get a new port number
                
                portMap[flow_input.src] = port;
		
                cout << "flow_input.src:" << flow_input.src << " port:" << port << endl;
		// Server
                uint16_t sinkPort = flow_input.dport; //TODO
                Address sinkAddress (InetSocketAddress (interfaces[CLIENT_CNT].GetAddress(1), sinkPort));
                PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort)); //TODO check NewReno
                ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(CLIENT_CNT + SERVER_CNT));
                sinkApp.Start (Seconds (0));
                sinkApp.Stop (Seconds (simulator_stop_time));
                // Client
                TypeId tid = TypeId::LookupByName ("ns3::TcpNewReno");
                Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));
                Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (flow_input.src), TcpSocketFactory::GetTypeId ());
                Ptr<MyApp> app = CreateObject<MyApp> ();
                app->Setup (ns3TcpSocket, sinkAddress, 1024, flow_input.maxPacketCount, DataRate (DATARATE)); // (TODO flow size) Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate
                nodes.Get(flow_input.src)->AddApplication (app);
                app->SetStartTime (Seconds (0.));
                //app->SetStopTime (Seconds (stopTime));

                // trace cwnd
                /*stringstream path;
                path << "GBResult/plotResult/" << "flow" << flow_input.idx << ".cwnd";
                Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (path.str());
                ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));*/

		// get the next flow input
		flow_input.idx++;
		ReadFlowInput();
	}

	// schedule the next time to run this function
	if (flow_input.idx < flow_num){
                cout << "flow_input.idx:" << flow_input.idx << " Seconds(flow_input.start_time):" << Seconds(flow_input.start_time) << " Simulator::Now():" << Simulator::Now() << " Seconds(flow_input.start_time)-Simulator::Now():" << Seconds(flow_input.start_time)-Simulator::Now() << endl;
		Simulator::Schedule(Seconds(flow_input.start_time)-Simulator::Now(), ScheduleFlowInputs);
	}
        else { // no more flows, close the file
		flowf.close();
	}
}

int GetHostNumber(Ipv4Address ip){
        int hostNo,j =0;
        //convert IPV4address into string
        stringstream ss;
        ss << ip;
        string str = ss.str();
        //Get HostNumber
        char input[1000];
	for(int i = 0; i < (int)str.length(); i++){
	    	input[i] = str[i];
	}
	input[str.length()] = '\0';

        const char *d = ".";
        char *p;
        p = strtok(input,d);
        while(p)
        {
                j++;
                if(j==3){
                       hostNo = atoi(p); 
                }
                p=strtok(NULL,d);
        }
        return hostNo;
        
}

static void
ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> monitor)
{
  //FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  double localThrou = 0.0;
  double localThroutx = 0.0;
  double totalLocalThrou = 0.0;
  double totalLocalThroutx = 0.0;
  //int totalLostPkts = 0;
  int totalRxPkts = 0;
  int totalTxPkts = 0;
  Time delaySum = Seconds(0);

  //double localThroutx = 0.0;
  monitor->CheckForLostPackets ();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  Time curTime = Now ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
 	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        // record to corresponding flow
        int src_No = GetHostNumber(t.sourceAddress);
        //int dst_No = GetHostNumber(t.destinationAddress);
        uint16_t src_port = t.sourcePort;
        uint32_t k = (src_No - 1)<<16 | src_port;
        //int k = 0; 


        stringstream dstip;
        dstip << "10.1." << CLIENT_CNT + SERVER_CNT << ".2";
        //dstip << "10.1.1.2";
        //cout << srcip.str() << endl;
        if (t.destinationAddress == Ipv4Address(dstip.str().c_str())){
                        localThrou = 8 * (i->second.rxBytes - previous[k]) / (1024 * 1024 * (curTime.GetSeconds () - prevTime[k].GetSeconds ())); 
                        localThroutx =  8 * (i->second.txBytes - previoustx[k]) / (1024 * 1024 * (curTime.GetSeconds () - prevTime[k].GetSeconds ()));
                        stringstream path;
                        path << "GBResult/performance" << src_No << ".dat"; //plotResult
                        std::ofstream thr0(path.str(), std::ios::out | std::ios::app);
		        //thr0 <<  curTime.GetSeconds () << " Trx:" << localThrou << " rxBytes:" << i->second.rxBytes << "  Ttx:" << localThroutx << " txBytes:" << i->second.txBytes << " drop:" << i->second.txPackets - i->second.rxPackets << std::endl;
                        thr0 <<  curTime.GetSeconds () << " " << localThrou << " rxBytes:" << i->second.rxBytes - previous[k] << " txBytes:" << i->second.txBytes - previoustx[k] << " rxPkts:" << i->second.rxPackets << " txPkts:" << i->second.txPackets << std::endl;
		        cout << "S:" << curTime.GetSeconds() << endl;
		        prevTime[k] = curTime;
		        previous[k] = i->second.rxBytes;
                        previoustx[k] = i->second.txBytes;
		        cout << "==========" << src_No << "_" << src_port << "=============" << endl; 
                        totalLocalThrou += localThrou;
                        totalLocalThroutx += localThroutx;
                        //totalLostPkts = i->second.txPackets - i->second.rxPackets;
                        totalRxPkts += i->second.rxPackets;
                        totalTxPkts += i->second.txPackets;
                        delaySum += i->second.delaySum;
        }
  }
  // write total throughput value into the file 
  stringstream path;
  path << "GBResult/performance.dat"; //plotResult
  stringstream path2;
  path2 << "GBResult/performance_detail.dat"; //plotResult
  std::ofstream thr(path.str(), std::ios::out | std::ios::app);
  std::ofstream thr2(path2.str(), std::ios::out | std::ios::app);
  if (totalRxPkts != 0){
       thr2 << curTime.GetSeconds () << "s  throuput:" << totalLocalThrou << " tx:" << totalLocalThroutx << "  totalLostPkts:" << (totalTxPkts-totalRxPkts) << "  avg_delay:" << delaySum.GetSeconds() / totalRxPkts << std::endl;
       thr << curTime.GetSeconds () << " " << totalLocalThrou << std::endl;
  }
  else{
        thr2 << curTime.GetSeconds () << "s  throuput:" << totalLocalThrou << " tx:" << totalLocalThroutx << "  totalRxPkts:" << totalRxPkts << "  totalLostPkts:" << (totalTxPkts-totalRxPkts) << std::endl;
        thr << curTime.GetSeconds () << " " << totalLocalThrou << std::endl;
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

        //flowf.open("/home/pc/ns-allinone-3.26/ns-3.26/scratch/flow_test.txt");
        flowf.open("scratch/traffic_test.txt");
        //flowf.open("scratch/flow_test.txt");
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
        // client -- router
        for (int i = 0; i < CLIENT_CNT; i++){      
                p2p[i].SetDeviceAttribute ("DataRate", StringValue (DATARATE)); // Bandwith
                p2p[i].SetChannelAttribute ("Delay", StringValue (DELAY));  // Delay
                p2p[i].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(500)); // Buffer of transmission queue
        } 
        // router -- server   
        p2p[CLIENT_CNT + SERVER_CNT - 1].SetDeviceAttribute ("DataRate", StringValue (DATARATE)); // Bandwith
        p2p[CLIENT_CNT + SERVER_CNT - 1].SetChannelAttribute ("Delay", StringValue (DELAY));  // Delay
        p2p[CLIENT_CNT + SERVER_CNT - 1].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1)); // Buffer of transmission queue
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

        //tch.SetRootQueueDisc ("ns3::SpPifo");
        //tch.SetRootQueueDisc ("ns3::Fifo");
        tch.SetRootQueueDisc ("ns3::Gearbox_pl_fid_flex");
        //tch.SetRootQueueDisc ("ns3::AFQ");
        tch.Install(devices[CLIENT_CNT + SERVER_CNT - 1].Get(0));
  
        // Create router nodes, initialize routing database and set up the routing tables in the nodes.
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// maintain port number for each host pair
	

        flow_input.idx = 0;
       	if (flow_num > 0){
		ReadFlowInput();
		Simulator::Schedule(Seconds(flow_input.start_time)-Simulator::Now(), ScheduleFlowInputs);
	}



        // monitor throughput (TODO: Flow: src, dst, port)
        FlowMonitorHelper flowmon;
        Ptr<FlowMonitor> monitor = flowmon.Install(nodes);

        ThroughputMonitor (&flowmon, monitor);

        // create trace file
        /*AsciiTraceHelper ascii;
        p2p[CLIENT_CNT + SERVER_CNT - 1].EnableAsciiAll(ascii.CreateFileStream("GBResult/plotResult/RouterTCPTopo.tr"));
        p2p[0].EnableAsciiAll(ascii.CreateFileStream("GBResult/plotResult/1TCPTopo.tr"));
        p2p[1].EnableAsciiAll(ascii.CreateFileStream("GBResult/plotResult/2TCPTopo.tr"));
        p2p[2].EnableAsciiAll(ascii.CreateFileStream("GBResult/plotResult/3TCPTopo.tr"));*/

        // record drop
        /*PcapHelper pcapHelper;
        Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("GBResult/plotResult/RouterTCPtopo.pcap", std::ios::out, PcapHelper::DLT_PPP);
        devices[CLIENT_CNT + SERVER_CNT - 1].Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

        PcapHelper pcapHelper2;
        Ptr<PcapFileWrapper> file2 = pcapHelper2.CreateFile ("GBResult/plotResult/1TCPtopo.pcap", std::ios::out, PcapHelper::DLT_PPP);
        devices[0].Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file2));
        */

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
