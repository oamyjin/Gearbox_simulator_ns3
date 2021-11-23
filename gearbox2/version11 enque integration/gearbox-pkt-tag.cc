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
  return 20;
}
void 
GearboxPktTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU16 (departureRound);
  buf.WriteU16 (uid);
  buf.WriteU8 (overflow);
  buf.WriteU8 (migration);
  buf.WriteU8 (reload);
  buf.WriteU8 (fifoenque);
  buf.WriteU8 (fifodeque);
  buf.WriteU8 (pifoenque);
  buf.WriteU8 (pifodeque);
  buf.WriteU32 (enquetime);
  buf.WriteU32 (dequetime);
}
void 
GearboxPktTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  departureRound = buf.ReadU16 ();
  uid = buf.ReadU16();
  overflow = buf.ReadU8();
  migration = buf.ReadU8();
  reload = buf.ReadU8();
  fifoenque = buf.ReadU8();
  fifodeque = buf.ReadU8();
  pifoenque = buf.ReadU8();
  pifodeque = buf.ReadU8();
  enquetime = buf.ReadU32();
  dequetime = buf.ReadU32();
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

GearboxPktTag::GearboxPktTag (int round, float time, int id)
  : Tag (),
    departureRound (round), enquetime(time), uid(id)
{ //cout<<"tag"<<time<<" "<<enquetime;
  NS_LOG_FUNCTION (this << departureRound << " " << enquetime);
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
GearboxPktTag::SetOverflow (int overflow)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << overflow);
  this->overflow = overflow;
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
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << migration);
  this->migration = migration;
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
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << reload);
  this->reload = reload;
}

int
GearboxPktTag::GetReload (void) const
{
  NS_LOG_FUNCTION (this);
  return reload;
}
int
GearboxPktTag::SetFifoenque (int fifoenque)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << fifoenque);
  this->fifoenque = fifoenque;
  return fifoenque;
}

int
GearboxPktTag::GetFifoenque (void) const
{
  NS_LOG_FUNCTION (this);
  return fifoenque;
}


void
GearboxPktTag::SetFifodeque (int fifodeque)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << fifodeque);
  this->fifodeque = fifodeque;
}

int
GearboxPktTag::GetFifodeque (void) const
{
  NS_LOG_FUNCTION (this);
  return fifodeque;
}
void
GearboxPktTag::SetPifoenque (int pifoenque)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << pifoenque);
  this->pifoenque = pifoenque;
}

int
GearboxPktTag::GetPifoenque (void) const
{
  NS_LOG_FUNCTION (this);
  return pifoenque;
}
void
GearboxPktTag::SetPifodeque(int pifodeque)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << pifodeque);
  this->pifodeque = pifodeque;
}

int
GearboxPktTag::GetPifodeque (void) const
{
  NS_LOG_FUNCTION (this);
  return pifodeque;
}

void
GearboxPktTag::SetEnqueTime(float enquetime)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << pifodeque);
  this->enquetime = enquetime;
}

float
GearboxPktTag::GetEnqueTime (void) const
{
  NS_LOG_FUNCTION (this);
  //cout<<"tag get"<<enquetime<<endl;
  return enquetime;
}
void
GearboxPktTag::SetDequeTime(float dequetime)
{
  ////cout<<"overflow"<<overflow<<endl;
  NS_LOG_FUNCTION (this << pifodeque);
  this->dequetime = dequetime;
}

float
GearboxPktTag::GetDequeTime (void) const
{
  NS_LOG_FUNCTION (this);
  return dequetime;
}

} // namespace ns3
