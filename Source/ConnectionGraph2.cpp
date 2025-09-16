/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <RakNet/NativeFeatureIncludes.h>
#if _RAKNET_SUPPORT_ConnectionGraph2==1

#include <RakNet/ConnectionGraph2.h>
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/BitStream.h>

using namespace RakNet;

STATIC_FACTORY_DEFINITIONS(ConnectionGraph2,ConnectionGraph2)

int RakNet::ConnectionGraph2::RemoteSystemComp( const RakNetGUID &key, RemoteSystem * const &data )
{
	if (key < data->guid)
		return -1;
	if (key > data->guid)
		return 1;
	return 0;
}

int RakNet::ConnectionGraph2::SystemAddressAndGuidComp( const SystemAddressAndGuid &key, const SystemAddressAndGuid &data )
{
	if (key.guid<data.guid)
		return -1;
	if (key.guid>data.guid)
		return 1;
	return 0;
}
ConnectionGraph2::ConnectionGraph2()
{
	autoProcessNewConnections=true;
}
ConnectionGraph2::~ConnectionGraph2()
{

}
bool ConnectionGraph2::GetConnectionListForRemoteSystem(
    const RakNetGUID& remoteSystemGuid, SystemAddress *saOut, RakNetGUID *guidOut, unsigned int *outLength)
{
	if ((saOut==nullptr && guidOut==nullptr) || outLength==nullptr || *outLength==0 || remoteSystemGuid==UNASSIGNED_RAKNET_GUID)
	{
		*outLength=0;
		return false;
	}

	bool objectExists;
  const unsigned int idx = remoteSystems.GetIndexFromKey(remoteSystemGuid, &objectExists);
	if (objectExists==false)
	{
		*outLength=0;
		return false;
	}

  if (remoteSystems[idx]->remoteConnections.Size() < *outLength)
		*outLength=remoteSystems[idx]->remoteConnections.Size();
	for (unsigned int idx2 = 0; idx2 < *outLength; ++idx2)
	{
		if (guidOut)
			guidOut[idx2]=remoteSystems[idx]->remoteConnections[idx2].guid;
		if (saOut)
			saOut[idx2]=remoteSystems[idx]->remoteConnections[idx2].systemAddress;
	}
	return true;
}
bool ConnectionGraph2::ConnectionExists(const RakNetGUID& g1,
                                        const RakNetGUID& g2)
{
	if (g1==g2)
		return false;

	bool objectExists;
  const unsigned int idx = remoteSystems.GetIndexFromKey(g1, &objectExists);
	if (objectExists==false)
	{
		return false;
	}
	SystemAddressAndGuid sag;
	sag.guid=g2;
	return remoteSystems[idx]->remoteConnections.HasData(sag);
}
uint16_t ConnectionGraph2::GetPingBetweenSystems(const RakNetGUID& g1,
                                                 const RakNetGUID& g2) const
{
	if (g1==g2)
		return 0;

	if (g1==rakPeerInterface->GetMyGUID())
		return static_cast<uint16_t>(rakPeerInterface->GetAveragePing(g2));
	if (g2==rakPeerInterface->GetMyGUID())
		return static_cast<uint16_t>(rakPeerInterface->GetAveragePing(g1));

	bool objectExists;
  const unsigned int idx = remoteSystems.GetIndexFromKey(g1, &objectExists);
	if (objectExists==false)
	{
		return static_cast<uint16_t>(-1);
	}

	SystemAddressAndGuid sag;
	sag.guid = g2;
  const unsigned int idx2 = remoteSystems[idx]->remoteConnections.GetIndexFromKey(sag, &objectExists);
	if (objectExists==false)
	{
		return static_cast<uint16_t>(-1);
	}
	return remoteSystems[idx]->remoteConnections[idx2].sendersPingToThatSystem;
}

/// Returns the system with the lowest total ping among all its connections. This can be used as the 'best host' for a peer to peer session
RakNetGUID ConnectionGraph2::GetLowestAveragePingSystem() const
{
	float lowestPing=-1.0;
	auto lowestPingIdx= static_cast<unsigned int>(-1);
	float thisAvePing=0.0f;
	unsigned int idx;
	int ap, count=0;

	for (idx=0; idx<remoteSystems.Size(); ++idx)
	{
		thisAvePing=0.f;

		ap = rakPeerInterface->GetAveragePing(remoteSystems[idx]->guid);
		if (ap!=-1)
		{
			thisAvePing+= static_cast<float>(ap);
			count++;
		}
	}

	if (count>0)
	{
		lowestPing=thisAvePing/count;
	}

	for (idx=0; idx<remoteSystems.Size(); ++idx)
	{
		thisAvePing=0.0f;
		count = 0;

    const RemoteSystem *remoteSystem = remoteSystems[idx];
		for (unsigned int idx2 = 0; idx2 < remoteSystem->remoteConnections.Size(); ++idx2)
		{
			ap=remoteSystem->remoteConnections[idx2].sendersPingToThatSystem;
			if (ap!=-1)
			{
				thisAvePing+= static_cast<float>(ap);
				count++;
			}
		}

		if (count>0 && (lowestPing==-1.0f || thisAvePing/count < lowestPing))
		{
			lowestPing=thisAvePing/count;
			lowestPingIdx=idx;
		}
	}

	if (lowestPingIdx== static_cast<unsigned int>(-1))
		return rakPeerInterface->GetMyGUID();
	return remoteSystems[lowestPingIdx]->guid;
}

void ConnectionGraph2::OnClosedConnection(const SystemAddress &systemAddress, const RakNetGUID rakNetGUID,
    const PI2_LostConnectionReason lostConnectionReason )
{
	// Send notice to all existing connections
	RakNet::BitStream bs;
	if (lostConnectionReason==LCR_CONNECTION_LOST)
		bs.Write(static_cast<MessageID>(ID_REMOTE_CONNECTION_LOST));
	else
		bs.Write(static_cast<MessageID>(ID_REMOTE_DISCONNECTION_NOTIFICATION));
	bs.Write(systemAddress);
	bs.Write(rakNetGUID);
	SendUnified(&bs,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,true);

	bool objectExists;
  const unsigned int idx = remoteSystems.GetIndexFromKey(rakNetGUID, &objectExists);
	if (objectExists)
	{
		RakNet::OP_DELETE(remoteSystems[idx],_FILE_AND_LINE_);
		remoteSystems.RemoveAtIndex(idx);
	}
}
void ConnectionGraph2::SetAutoProcessNewConnections(bool b)
{
	autoProcessNewConnections=b;
}
bool ConnectionGraph2::GetAutoProcessNewConnections() const
{
	return autoProcessNewConnections;
}
void ConnectionGraph2::AddParticipant(const SystemAddress &systemAddress,
                                      const RakNetGUID& rakNetGUID)
{
	// Relay the new connection to other systems.
	RakNet::BitStream bs;
	bs.Write(static_cast<MessageID>(ID_REMOTE_NEW_INCOMING_CONNECTION));
	bs.Write(static_cast<uint32_t>(1));
	bs.Write(systemAddress);
	bs.Write(rakNetGUID);
	bs.WriteCasted<uint16_t>(rakPeerInterface->GetAveragePing(rakNetGUID));
	SendUnified(&bs,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,true);

	// Send everyone to the new guy
	DataStructures::List<SystemAddress> addresses;
	DataStructures::List<RakNetGUID> guids;
	rakPeerInterface->GetSystemList(addresses, guids);
	bs.Reset();
	bs.Write(static_cast<MessageID>(ID_REMOTE_NEW_INCOMING_CONNECTION));
  const BitSize_t writeOffset = bs.GetWriteOffset();
	bs.Write((uint32_t) addresses.Size());

  uint32_t count=0;
	for (unsigned int i = 0; i < addresses.Size(); ++i)
	{
		if (addresses[i]==systemAddress)
			continue;

		bs.Write(addresses[i]);
		bs.Write(guids[i]);
		bs.WriteCasted<uint16_t>(rakPeerInterface->GetAveragePing(guids[i]));
		++count;
	}

	if (count>0)
	{
    const BitSize_t writeOffset2 = bs.GetWriteOffset();
		bs.SetWriteOffset(writeOffset);
		bs.Write(count);
		bs.SetWriteOffset(writeOffset2);
		SendUnified(&bs,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,false);
	}

	bool objectExists;
  const unsigned int ii = remoteSystems.GetIndexFromKey(rakNetGUID, &objectExists);
	if (objectExists==false)
	{
		auto* remoteSystem = RakNet::OP_NEW<RemoteSystem>(_FILE_AND_LINE_);
		remoteSystem->guid=rakNetGUID;
		remoteSystems.InsertAtIndex(remoteSystem,ii,_FILE_AND_LINE_);
	}
}
void ConnectionGraph2::GetParticipantList(DataStructures::OrderedList<RakNetGUID, RakNetGUID> &participantList)
{
	participantList.Clear(true, _FILE_AND_LINE_);
  for (unsigned int i = 0; i < remoteSystems.Size(); ++i)
		participantList.InsertAtEnd(remoteSystems[i]->guid, _FILE_AND_LINE_);
}
void ConnectionGraph2::OnNewConnection(const SystemAddress &systemAddress,
                                       const RakNetGUID rakNetGUID,
                                       const bool isIncoming)
{
	(void) isIncoming;
	if (autoProcessNewConnections)
		AddParticipant(systemAddress, rakNetGUID);
}
PluginReceiveResult ConnectionGraph2::OnReceive(Packet *packet)
{
	if (packet->data[0]==ID_REMOTE_CONNECTION_LOST || packet->data[0]==ID_REMOTE_DISCONNECTION_NOTIFICATION)
	{
		bool objectExists;
    const unsigned idx = remoteSystems.GetIndexFromKey(packet->guid, &objectExists);
		if (objectExists)
		{
			RakNet::BitStream bs(packet->data,packet->length,false);
			bs.IgnoreBytes(1);
			SystemAddressAndGuid saag;
			bs.Read(saag.systemAddress);
			bs.Read(saag.guid);
      const unsigned long idx2 = remoteSystems[idx]->remoteConnections.GetIndexFromKey(saag, &objectExists);
			if (objectExists)
				remoteSystems[idx]->remoteConnections.RemoveAtIndex(idx2);
		}
	}
	else if (packet->data[0]==ID_REMOTE_NEW_INCOMING_CONNECTION)
	{
		bool objectExists;
    const unsigned idx = remoteSystems.GetIndexFromKey(packet->guid, &objectExists);
		if (objectExists)
		{
			uint32_t numAddresses;
			RakNet::BitStream bs(packet->data,packet->length,false);
			bs.IgnoreBytes(1);
			bs.Read(numAddresses);
			for (unsigned int idx2=0; idx2 < numAddresses; idx2++)
			{
				SystemAddressAndGuid saag;
				bs.Read(saag.systemAddress);
				bs.Read(saag.guid);
				bs.Read(saag.sendersPingToThatSystem);
				bool object_exists;
        const unsigned int ii = remoteSystems[idx]->remoteConnections.GetIndexFromKey(saag, &object_exists);
				if (object_exists==false)
					remoteSystems[idx]->remoteConnections.InsertAtIndex(saag,ii,_FILE_AND_LINE_);
			}
		}
	}
	
	return RR_CONTINUE_PROCESSING;
}

#endif // _RAKNET_SUPPORT_*
