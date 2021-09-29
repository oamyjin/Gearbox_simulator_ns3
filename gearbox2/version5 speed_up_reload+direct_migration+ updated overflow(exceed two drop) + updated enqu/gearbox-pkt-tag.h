/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef FLOW_ID_TAG_H
#define FLOW_ID_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class GearboxPktTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;
  GearboxPktTag ();
  
  /**
   *  Constructs a FlowIdTag with the given flow id
   *
   *  \param departure round and fifo index to use for the tag
   */
  GearboxPktTag (int departureRound, int index);
  /**
   *  Sets the departure round for the tag
   *  \param departure round to assign to the tag
   */
  void SetDepartureRound (int departureRound);
  /**
   *  Sets the fifo index for the tag
   *  \param fifo index to assign to the tag
   */
  void SetIndex (int index);
  /**
   *  Sets the overflow times for the tag
   *  \param overflow times to assign to the tag
   */
  void SetOverflow (int overflow);
  /**
   *  Gets the departure round for the tag
   *  \returns current departure round for this tag
   */
  int GetDepartureRound (void) const;
  /**
   *  Gets the fifo index for the tag
   *  \returns fifo index for this tag
   */
  int GetIndex (void) const;
  /**
   *  Gets the overflow times for the tag
   *  \returns overflow times for this tag
   */
  int GetOverflow (void) const;
private:
  int departureRound; //!< Flow ID
  int index; // fifo index
  int overflow=0;//overflow times
};

} // namespace ns3

#endif /* FLOW_ID_TAG_H */
