/* 
 * Packet Tag for Gearbox2
 * A packet tag includes 'packet departure round' and expected 'fifo index'
 * Author: Jiajin
 */
#include "gearbox-pkt-tag.h"
#include "ns3/log.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GearboxPktTag");

NS_OBJECT_ENSURE_REGISTERED (GearboxPktTag);

TypeId 
GearboxPktTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GearboxPktTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<GearboxPktTag> ()
  ;
  return tid;
}
TypeId 
GearboxPktTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
GearboxPktTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 6;
}
void 
GearboxPktTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU16 (departureRound);
  buf.WriteU16 (uid);
  buf.WriteU16 (flowNo);
}
void 
GearboxPktTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  departureRound = buf.ReadU16 ();
  uid = buf.ReadU16();
  flowNo = buf.ReadU16();
}
void 
GearboxPktTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "departureRound=" << departureRound;
}
GearboxPktTag::GearboxPktTag ()
  : Tag () 
{
  NS_LOG_FUNCTION (this);
}

GearboxPktTag::GearboxPktTag (int flNo, int id ,int round)
  : Tag (),
    flowNo(flNo), uid(id), departureRound (round)
{ //cout<<"tag"<<time<<" "<<enquetime;
  NS_LOG_FUNCTION (this << departureRound);
}

void
GearboxPktTag::SetDepartureRound (int round)
{
  NS_LOG_FUNCTION (this << round);
  departureRound = round;
}
int
GearboxPktTag::GetDepartureRound (void) const
{
  NS_LOG_FUNCTION (this);
  return departureRound;
}
void
GearboxPktTag::SetUid (int uid)
{
  NS_LOG_FUNCTION (this << uid);
  this->uid = uid;
}
int
GearboxPktTag::GetUid (void) const
{
  NS_LOG_FUNCTION (this);
  return uid;
}
void
GearboxPktTag::SetFlowNo (int flNo)
{
  NS_LOG_FUNCTION (this << flNo);
  this->flowNo = flNo;
}
uint64_t
GearboxPktTag::GetFlowNo (void) const
{
  NS_LOG_FUNCTION (this);
  return flowNo;
}

} // namespace ns3
