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
   A(1) --------
               |  
   A(2) -----\ |
              \| 
   A(3) ------ router(0) ------- S(6)
              /|
   A(4) -----/ |  
               |
   A(5) --------
 */

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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyTopoUDP");

uint32_t previous1 = 0;
uint32_t previous2 = 0;
uint32_t previous3 = 0;
Time prevTime1 = Seconds (0);
Time prevTime2 = Seconds (0);
Time prevTime3 = Seconds (0);
double nSamplingPeriod = 0.1;
double stopTime = 10.0; 

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
		std::ofstream thr ("UDPResult/14throughput.dat", std::ios::out | std::ios::app);
                localThrou = 8 * (i->second.rxBytes - previous1) / (1024 * 1024 * (curTime.GetSeconds () - prevTime1.GetSeconds ())); 
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime1 = curTime;
		previous1 = i->second.rxBytes;
		cout << "here==========1=============" << endl;
   	}
        if ((t.sourceAddress=="10.1.2.2" && t.destinationAddress == "10.1.4.2")){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("UDPResult/24throughput.dat", std::ios::out | std::ios::app);
                localThrou = 8 * (i->second.rxBytes - previous2) / (1024 * 1024 * (curTime.GetSeconds () - prevTime2.GetSeconds ()));
		thr <<  curTime.GetSeconds () << " " << localThrou << std::endl;
		cout << "S:" << curTime.GetSeconds() << endl;
		prevTime2 = curTime;
		previous2 = i->second.rxBytes;
		cout << "here==========2=============" << endl;
   	}
        if ((t.sourceAddress=="10.1.3.2" && t.destinationAddress == "10.1.4.2")){
        	//auto itr = flowStats.begin ();
		std::ofstream thr ("UDPResult/34throughput.dat", std::ios::out | std::ios::app);
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
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("DropTailQueue", LOG_LEVEL_INFO);

  //创建节点
  NodeContainer clientNodes, routers, serverNode;
  routers.Create (1);// 0
  clientNodes.Create (3);// 1,2,3
  serverNode.Create(1);// 4
  NodeContainer nodes = NodeContainer(routers, clientNodes, serverNode);
  //各条边的节点组合,一共12条边
  std::vector<NodeContainer> nodeAdjacencyList(4);  
  nodeAdjacencyList[0]=NodeContainer(nodes.Get(0),nodes.Get(1));//router(0),A(1)
  nodeAdjacencyList[1]=NodeContainer(nodes.Get(0),nodes.Get(2));//router(0),A(2)
  nodeAdjacencyList[2]=NodeContainer(nodes.Get(0),nodes.Get(3));//router(0),A(3)
  nodeAdjacencyList[3]=NodeContainer(nodes.Get(0),nodes.Get(4));//router(0),S(4)

  /*配置信道*/
  std::vector<PointToPointHelper> p2p(4);  
  p2p[0].SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));//设置带宽
  p2p[0].SetChannelAttribute ("Delay", StringValue ("1ms"));  //设置时延
  p2p[0].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1000));
  //高速路段
  for(int i = 1; i < 4; i ++){
    p2p[i].SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));//设置带宽
    p2p[i].SetChannelAttribute ("Delay", StringValue ("1ms"));  //设置时延
    p2p[i].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1000));
  }
  //瓶颈路段
  p2p[3].SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p[3].SetChannelAttribute ("Delay", StringValue ("2ms"));  
  p2p[3].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(1000));
 

  std::vector<NetDeviceContainer> devices(4);
  for(int i = 0; i < 4; i ++){
    devices[i] = p2p[i].Install(nodeAdjacencyList[i]);
  }

 /*安装网络协议栈*/
  InternetStackHelper stack;  
  stack.Install (nodes);//安装协议栈，tcp、udp、ip等  

  //NS_LOG_INFO ("Assign IP address.");
  Ipv4AddressHelper address;  
  std::vector<Ipv4InterfaceContainer> interfaces(4);  
  for(uint32_t i=0; i<4; i++)  
  {  
    std::ostringstream subset;  
    subset<<"10.1."<<i+1<<".0"; 
    //n0:10.1.1.1    n1接口1:10.1.1.2   n1接口2:10.1.2.1  n2接口1:10.1.2.2  ...
    address.SetBase(subset.str().c_str (),"255.255.255.0");//设置基地址（默认网关）、子网掩码  
    interfaces[i]=address.Assign(devices[i]);//把IP地址分配给网卡,ip地址分别是10.1.1.1和10.1.1.2，依次类推  
  }  

  TrafficControlHelper tch;
  //Config::SetDefault ("ns3::DynamicQueueLimits::MaxPackets", UintegerValue (10));
  //tch.SetQueueLimits ("ns3::DynamicQueueLimits", "MinLimit", StringValue ("1ms"));
  tch.Uninstall(devices[0]);
  tch.Uninstall(devices[1]);
  tch.Uninstall(devices[2]);
  tch.Uninstall(devices[3]);

  //Config::SetDefault ("ns3::DynamicQueueLimits::MaxPackets", UintegerValue (10));
  tch.SetRootQueueDisc ("ns3::Gearbox_pl_fid_flex");
  //tch.SetRootQueueDisc ("ns3::AFQ_10_unlim_pl");
  tch.Install(devices[3]);
  

  // Create router nodes, initialize routing database and set up the routing tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(4));
  sinkApp.Start (Seconds (0));
  sinkApp.Stop (Seconds (stopTime+1));

  AddressValue remoteAddress (InetSocketAddress (interfaces[3].GetAddress (1), port));
  OnOffHelper sourceHelper ("ns3::UdpSocketFactory", Address ());
  sourceHelper.SetAttribute ("DataRate", StringValue ("4Mbps"));
  sourceHelper.SetAttribute ("PacketSize", UintegerValue (1024));
  sourceHelper.SetAttribute ("Remote", remoteAddress);
  sourceHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));  
  sourceHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]")); 

  ApplicationContainer sourceApp1 = sourceHelper.Install (nodes.Get(1)); //A1
  ApplicationContainer sourceApp2 = sourceHelper.Install (nodes.Get(2)); //A2
  ApplicationContainer sourceApp3 = sourceHelper.Install (nodes.Get(3)); //A3
  sourceApp1.Start (Seconds (0));
  sourceApp1.Stop (Seconds (stopTime));
  sourceApp2.Start (Seconds (0));
  sourceApp2.Stop (Seconds (stopTime));
  sourceApp3.Start (Seconds (0));
  sourceApp3.Stop (Seconds (stopTime));

  /*
  AsciiTraceHelper ascii;
  p2p[0].EnableAsciiAll (ascii.CreateFileStream ("UDPResult/first_pip1.tr"));
  p2p[1].EnableAsciiAll (ascii.CreateFileStream ("UDPResult/first_pip2.tr"));
  p2p[2].EnableAsciiAll (ascii.CreateFileStream ("UDPResult/first_pip3.tr"));
  p2p[3].EnableAsciiAll (ascii.CreateFileStream ("UDPResult/first_pip4.tr"));
  */

  /*for(uint32_t i=0; i<4; i++)  
      p2p[i].EnablePcapAll("UDPResult/mytest"); */


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.Install(nodes);

  Simulator::Stop (Seconds(stopTime));

  ThroughputMonitor (&flowmon, monitor);

  Simulator::Run ();  
  Simulator::Destroy ();
  return 0;
}

