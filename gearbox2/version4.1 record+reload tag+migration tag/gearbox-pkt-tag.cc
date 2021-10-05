/* 
 * Packet Tag for Gearbox2
 * A packet tag includes 'packet departure round' and expected 'fifo index'
 * Author: Jiajin
 */
#include "gearbox-pkt-tag.h"
#include "ns3/log.h"

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
  return 8;
}
void 
GearboxPktTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU32 (departureRound);
  buf.WriteU32 (index);
}
void 
GearboxPktTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  departureRound = buf.ReadU32 ();
  index = buf.ReadU32();
}
void 
GearboxPktTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "departureRound=" << departureRound << " index=" << index<<" overflow="<<overflow;
}
GearboxPktTag::GearboxPktTag ()
  : Tag () 
{
  NS_LOG_FUNCTION (this);
}

GearboxPktTag::GearboxPktTag (int round, int ind)
  : Tag (),
    departureRound (round), index(ind)
{
  NS_LOG_FUNCTION (this << departureRound << " " << index);
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
GearboxPktTag::SetIndex (int ind)
{
  NS_LOG_FUNCTION (this << ind);
  index = ind;
}
int
GearboxPktTag::GetIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return index;
}
void
GearboxPktTag::SetOverflow (int overflow)
{
  //cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << overflow);
  overflow = overflow;
}

int
GearboxPktTag::GetOverflow (void) const
{
  NS_LOG_FUNCTION (this);
  return overflow;
}

void
GearboxPktTag::SetMigration (int migration)
{
  //cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << migration);
  migration = migration;
}

int
GearboxPktTag::GetMigration (void) const
{
  NS_LOG_FUNCTION (this);
  return migration;
}

void
GearboxPktTag::SetReload (int reload)
{
  //cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << reload);
  reload = reload;
}

int
GearboxPktTag::GetReload (void) const
{
  NS_LOG_FUNCTION (this);
  return reload;
}
} // namespace ns3
