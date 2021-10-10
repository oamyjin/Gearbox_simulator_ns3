/*
         A(1)
           \  
            \   
             \ 
   A(2) ------ router(0) ------- S(4)
             / 
            /  
           /   
          A(3)
(i)为程序中NodeContainer nodes中节点的顺序号
*/
 
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/stats-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/netanim-module.h"
 
using namespace ns3;
using namespace std;
 
NS_LOG_COMPONENT_DEFINE ("myTopoTCPAsyPlot");

uint32_t previous1 = 0;
uint32_t previous2 = 0;
uint32_t previous3 = 0;
Time prevTime0 = Seconds (0);
Time prevTime1 = Seconds (0);
Time prevTime2 = Seconds (0);
Time prevTime3 = Seconds (0);
double nSamplingPeriod = 0.1; 
double stopTime = 10.0;

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

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  //*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << Simulator::Now ().GetSeconds () - prevTime1.GetSeconds () << "\t" << newCwnd << std::endl;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;

}


static void
ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> monitor)
{
  //FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  double localThrou = 0.0;
  monitor->CheckForLostPackets ();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); i++){
 	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	Time curTime = Now ();
	if ((t.sourceAddress=="10.1.1.2" && t.destinationAddress == "10.1.4.2")){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("GBResultAsy2/14throughput.dat", std::ios::out | std::ios::app);
		cout << "t:" << curTime.GetSeconds () << " t':" << prevTime1.GetSeconds () << " previous1:" << previous1 << endl;
                localThrou = 8 * (i->second.rxBytes - previous1) / (1024 * 1024 * nSamplingPeriod);//(curTime.GetSeconds () - prevTime1.GetSeconds ())
		cout << "1 i->second.rxBytes:" << i->second.rxBytes << endl;
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime1 = curTime;
		previous1 = i->second.rxBytes;
		cout << "here==========1=============" << endl;
   	}
        if (t.sourceAddress=="10.1.2.2" && t.destinationAddress == "10.1.4.2"){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("GBResultAsy2/24throughput.dat", std::ios::out | std::ios::app);
		cout << "t:" << curTime.GetSeconds () - prevTime2.GetSeconds () << " previous2:" << previous2 << endl;
                localThrou = 8 * (i->second.rxBytes - previous2) / (1024 * 1024 * (curTime.GetSeconds () - prevTime2.GetSeconds ()));		
		cout << "2 i->second.rxBytes:" << i->second.rxBytes << endl;
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime2 = curTime;
		previous2 = i->second.rxBytes;
		cout << "here==========2=============" << endl;
   	}
        if (t.sourceAddress=="10.1.3.2" && t.destinationAddress == "10.1.4.2"){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("GBResultAsy2/34throughput.dat", std::ios::out | std::ios::app);
                localThrou = 8 * (i->second.rxBytes - previous3) / (1024 * 1024 * (curTime.GetSeconds () - prevTime3.GetSeconds ()));
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime3 = curTime;
		previous3 = i->second.rxBytes;
		cout << "here==========3=============" << endl;
   	}
  }
  Simulator::Schedule (Seconds(nSamplingPeriod), &ThroughputMonitor, fmhelper, monitor);
}


int
main (int argc, char * argv[])
{

  Time::SetResolution(Time::NS);
  //LogComponentEnable ("TrafficControlHelper", LOG_LEVEL_INFO);
  //LogComponentEnable ("Gearbox_pl_fid_flex", LOG_LEVEL_INFO);
  //LogComponentEnable ("DropTailQueue", LOG_LEVEL_INFO);
 
  /*使用可视化工具 PyViz*/
  CommandLine cmd;
  cmd.Parse (argc,argv);
 
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));

  //创建节点
  NodeContainer clientNodes, router, serverNode;
  router.Create (1);// 0
  clientNodes.Create (3);// 1,2,3
  serverNode.Create(1);// 4
  NodeContainer nodes = NodeContainer(router, clientNodes, serverNode);
  
  std::vector<NodeContainer> nodeAdjacencyList(4);  
  nodeAdjacencyList[0]=NodeContainer(nodes.Get(0),nodes.Get(1));//router(0),A(1)
  nodeAdjacencyList[1]=NodeContainer(nodes.Get(0),nodes.Get(2));//router(0),A(2)
  nodeAdjacencyList[2]=NodeContainer(nodes.Get(0),nodes.Get(3));//router(0),A(3)
  nodeAdjacencyList[3]=NodeContainer(nodes.Get(0),nodes.Get(4));//router(0),S(4)

  /*配置信道*/
  std::vector<PointToPointHelper> p2p(4);  
  //高速路段
  p2p[0].SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p[0].SetChannelAttribute ("Delay", StringValue ("1ms"));
  p2p[0].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1000));
  for(int i = 1; i < 3; i ++){
    p2p[i].SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
    p2p[i].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1000));
  }
  //瓶颈路段1
  p2p[3].SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p[3].SetChannelAttribute ("Delay", StringValue ("2ms"));  
  p2p[3].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1000));
 

  std::vector<NetDeviceContainer> devices(4);
  for(int i = 0; i < 4; i ++){
    devices[i] = p2p[i].Install(nodeAdjacencyList[i]);
  }

 /* // drop rate
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  devices[3].Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));*/

 /*安装网络协议栈*/
  InternetStackHelper stack;  
  stack.Install (nodes);  

  //NS_LOG_INFO ("Assign IP address.");
  Ipv4AddressHelper address;  
  std::vector<Ipv4InterfaceContainer> interfaces(4);  
  for(uint32_t i=0; i<4; i++)  
  {  
    std::ostringstream subset;  
    subset<<"10.1."<<i+1<<".0"; 
    address.SetBase(subset.str().c_str (),"255.255.255.0");  
    interfaces[i]=address.Assign(devices[i]);
  }  

  TrafficControlHelper tch;
  tch.Uninstall(devices[0]);
  tch.Uninstall(devices[1]);
  tch.Uninstall(devices[2]);
  tch.Uninstall(devices[3]);

  tch.SetRootQueueDisc ("ns3::Gearbox_pl_fid_flex");  
  //tch.SetRootQueueDisc ("ns3::AFQ_10_unlim_pl");
  tch.Install(devices[3].Get(0));
  
  // Server
  uint16_t sinkPort = 50000;
  Address sinkAddress (InetSocketAddress (interfaces[3].GetAddress(1), sinkPort));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(4));
  sinkApp.Start (Seconds (0));
  sinkApp.Stop (Seconds (stopTime+1));

  // Client
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 100000, DataRate ("10Mbps")); //Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate
  nodes.Get(1)->AddApplication (app);
  app->SetStartTime (Seconds (0.));
  app->SetStopTime (Seconds (stopTime));

  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (nodes.Get (2), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup (ns3TcpSocket2, sinkAddress, 1040, 100000, DataRate ("10Mbps")); //Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate
  nodes.Get(2)->AddApplication (app2);
  app2->SetStartTime (Seconds (0.));
  app2->SetStopTime (Seconds (stopTime));

  Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (nodes.Get (3), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app3 = CreateObject<MyApp> ();
  app3->Setup (ns3TcpSocket3, sinkAddress, 1040, 100000, DataRate ("10Mbps")); //Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate
  nodes.Get(3)->AddApplication (app3);
  app3->SetStartTime (Seconds (0.));
  app3->SetStopTime (Seconds (stopTime));

  // trace cwnd
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("GBResultAsy2/app1.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("GBResultAsy2/app2.cwnd");
  ns3TcpSocket2->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream2));
  Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream ("GBResultAsy2/app3.cwnd");
  ns3TcpSocket3->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream3));



  /*
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));  
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]")); */
 
  /*接下来是建立通信的app和路由表的建立*/  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Check for dropped packets using Flow Monito
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds(stopTime));

  ThroughputMonitor (&flowmon, monitor);
  //嗅探,记录所有节点相关的数据包  
  /*for(uint32_t i=0; i<4; i++)  
      p2p[i].EnablePcapAll("GBResultAsy/myTopo");

  AsciiTraceHelper ascii;
  p2p[0].EnableAsciiAll (ascii.CreateFileStream ("GBResultAsy/mytopo-pip0.tr")); */  

  std::cout << "run"<<std::endl;
  Simulator::Run ();  
  std::cout << "destroy"<<std::endl;

  Simulator::Destroy ();  
  std::cout << "finished"<<std::endl;
  return 0;  
}
