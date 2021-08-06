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
 
NS_LOG_COMPONENT_DEFINE ("myTopoTCP");

uint32_t previous1 = 0;
uint32_t previous2 = 0;
uint32_t previous3 = 0;
Time prevTime1 = Seconds (0);
Time prevTime2 = Seconds (0);
Time prevTime3 = Seconds (0);
double nSamplingPeriod = 1; 
double stopTime = 20.0;

static void
ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> monitor)
{
  //FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  double localThrou = 0.0;
  monitor->CheckForLostPackets ();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
 	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	Time curTime = Now ();
	if ((t.sourceAddress=="10.1.1.2" && t.destinationAddress == "10.1.4.2")){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("GBResult/14throughput.dat", std::ios::out | std::ios::app);
                localThrou = 8 * (i->second.rxBytes - previous1) / (1024 * 1024 * (curTime.GetSeconds () - prevTime1.GetSeconds ())); 
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime1 = curTime;
		previous1 = i->second.rxBytes;
		cout << "here==========1=============" << endl;
   	}
        if ((t.sourceAddress=="10.1.2.2" && t.destinationAddress == "10.1.4.2")){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("GBResult/24throughput.dat", std::ios::out | std::ios::app);
                localThrou = 8 * (i->second.rxBytes - previous2) / (1024 * 1024 * (curTime.GetSeconds () - prevTime2.GetSeconds ()));
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime2 = curTime;
		previous2 = i->second.rxBytes;
		cout << "here==========2=============" << endl;
   	}
        if ((t.sourceAddress=="10.1.3.2" && t.destinationAddress == "10.1.4.2")){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("GBResult/34throughput.dat", std::ios::out | std::ios::app);
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
  tch.Install(devices[3]);
  
  /*在n7,n9,n11的8080端口建立SinkApplication
  发数据 n0——n7   （背景流量 n8——n9  n10——n11）
  sink接收TCP流数据
  */
  uint16_t servPort = 50000;
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), servPort));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(4));
  sinkApp.Start (Seconds (0));
  sinkApp.Stop (Seconds (stopTime));

 
  /*安装client*/
  uint16_t port = 50000; 
 
  ApplicationContainer clientApps1, clientApps2, clientApps3;  
  AddressValue remoteAddress(InetSocketAddress (interfaces[3].GetAddress (1), port));

  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address (InetSocketAddress (interfaces[3].GetAddress (1), port))); 
  clientHelper.SetAttribute("Remote",remoteAddress); 
  //OnOffApplication类中的m_onTime和m_offTime分别为发送持续的时间和不发送持续的时间。如下表示一直发送。
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));  
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]")); 

  clientHelper.SetConstantRate (DataRate ("10Mbps")); 
  clientApps1 = clientHelper.Install(nodes.Get(1));  //A1
  clientApps1.Start(Seconds(1.0)); 
  clientApps1.Stop(Seconds (stopTime-1));

  clientHelper.SetConstantRate (DataRate ("10Mbps"));
  clientApps2 = clientHelper.Install(nodes.Get(2));  //A2  
  clientApps3 = clientHelper.Install(nodes.Get(3));  //A2  

  clientApps2.Start(Seconds(1.0)); 
  clientApps2.Stop(Seconds (stopTime-1));
  clientApps3.Start(Seconds(1.0)); 
  clientApps3.Stop(Seconds (stopTime-1));
 
  /*接下来是建立通信的app和路由表的建立*/  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Check for dropped packets using Flow Monito
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds(stopTime));

  ThroughputMonitor (&flowmon, monitor);
  //嗅探,记录所有节点相关的数据包  
  for(uint32_t i=0; i<4; i++)  
      p2p[i].EnablePcapAll("GBResult/myTopo");

  AsciiTraceHelper ascii;
  p2p[0].EnableAsciiAll (ascii.CreateFileStream ("GBResult/mytopo-pip0.tr"));  

  std::cout << "run"<<std::endl;
  Simulator::Run ();  
  std::cout << "destroy"<<std::endl;

  Simulator::Destroy ();  
  std::cout << "finished"<<std::endl;
  return 0;  
}
