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
 
using namespace ns3;
using namespace std;
 
NS_LOG_COMPONENT_DEFINE ("myTopoTCP");

uint32_t previous = 0;
Time prevTime = Seconds (0);

static void
TraceThroughput (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr ("GBResult/throughput.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << 8 * (itr->second.txBytes - previous) / (1000 * 1000 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
  cout << "S:" << curTime.GetSeconds() << endl;
  prevTime = curTime;
  previous = itr->second.txBytes;
  Simulator::Schedule (Seconds (0.2), &TraceThroughput, monitor);
}

int
main (int argc, char * argv[])
{

  Time::SetResolution(Time::NS);
  //LogComponentEnable ("TrafficControlHelper", LOG_LEVEL_INFO);
  LogComponentEnable ("Gearbox_pl_fid_flex", LOG_LEVEL_INFO);
  //LogComponentEnable ("DropTailQueue", LOG_LEVEL_INFO);
 
  /*使用可视化工具 PyViz*/
  CommandLine cmd;
  cmd.Parse (argc,argv);
 
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));  //设置默认包的尺寸
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("1Gbps"));  //设置默认发包速率

  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
 // Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", QueueSizeValue (QueueSize ("100p")));


  //Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));

  //创建节点
  NodeContainer clientNodes, router, serverNode;
  router.Create (1);// 0
  clientNodes.Create (3);// 1,2,3
  serverNode.Create(1);// 4
  NodeContainer nodes = NodeContainer(router, clientNodes, serverNode);
  //各条边的节点组合,一共12条边
  std::vector<NodeContainer> nodeAdjacencyList(4);  
  nodeAdjacencyList[0]=NodeContainer(nodes.Get(0),nodes.Get(1));//router(0),A(1)
  nodeAdjacencyList[1]=NodeContainer(nodes.Get(0),nodes.Get(2));//router(0),A(2)
  nodeAdjacencyList[2]=NodeContainer(nodes.Get(0),nodes.Get(3));//router(0),A(3)
  nodeAdjacencyList[3]=NodeContainer(nodes.Get(0),nodes.Get(4));//router(0),S(4)


  /*配置信道*/
  std::vector<PointToPointHelper> p2p(4);  
  //高速路段
  for(int i = 0; i < 3; i ++){
    p2p[i].SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));//设置带宽
    p2p[i].SetChannelAttribute ("Delay", StringValue ("1ms"));  //设置时延
    p2p[i].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(50));
  }
  //瓶颈路段1
  p2p[3].SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p[3].SetChannelAttribute ("Delay", StringValue ("2ms"));  
  p2p[3].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(50));
  //p2p[3].SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(5));  
 

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
  sinkApp.Stop (Seconds (20.0));
  //uint16_t serverSum = 3;  //n7,n9,n11上安装serverApp,所以server一共3个
  //uint16_t servPort = 50000;  
  //ApplicationContainer sinkApp;  
  //Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), servPort));  //???
  //PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);   //第二个参数为上一行中的Address
  //sinkApp.Add(sinkHelper.Install(nodes.Get(4)));  //H4
  //sinkApp.Start (Seconds (0.0));  
  //sinkApp.Stop (Seconds (20.0));  //应该为60s

 
  /*安装client*/
  uint16_t port = 50000; 
 
  ApplicationContainer clientApps1, clientApps2, clientApps3;  
  AddressValue remoteAddress(InetSocketAddress (interfaces[3].GetAddress (1), port));  //目的地址 H4

  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address (InetSocketAddress (interfaces[3].GetAddress (1), port))); 
  clientHelper.SetConstantRate (DataRate ("1Gbps")); 
  clientHelper.SetAttribute("Remote",remoteAddress); 
  //OnOffApplication类中的m_onTime和m_offTime分别为发送持续的时间和不发送持续的时间。如下表示一直发送。
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));  
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]")); 

  clientApps1 = clientHelper.Install(nodes.Get(1));  //A1
  clientApps2 = clientHelper.Install(nodes.Get(2));  //A2  
  clientApps3 = clientHelper.Install(nodes.Get(3));  //A2  

  clientApps1.Start(Seconds(1.0)); 
  clientApps1.Stop(Seconds (10.0));
 
  clientApps2.Start(Seconds(1.0)); 
  clientApps2.Stop(Seconds (10.0));
 
  clientApps3.Start(Seconds(1.0)); 
  clientApps3.Stop(Seconds (10.0));
 
  /*接下来是建立通信的app和路由表的建立*/  
  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 

  // Check for dropped packets using Flow Monito
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);


  //嗅探,记录所有节点相关的数据包  
  for(uint32_t i=0; i<4; i++)  
      p2p[i].EnablePcapAll("TCPResult/myTopo");

  AsciiTraceHelper ascii;
  p2p[0].EnableAsciiAll (ascii.CreateFileStream ("TCPResult/mytopo-pip0.tr"));  

  
  std::cout << "last"<<std::endl;
  Simulator::Run ();  
  std::cout << "lastlastlast"<<std::endl;
  Simulator::Destroy ();  
  std::cout << "lastlast"<<std::endl;
  return 0;  
}
