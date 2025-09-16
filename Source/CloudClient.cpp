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
#if _RAKNET_SUPPORT_CloudClient == 1

#include <RakNet/CloudClient.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakPeerInterface.h>

using namespace RakNet;

STATIC_FACTORY_DEFINITIONS(CloudClient,CloudClient);

CloudClient::CloudClient()
{
	callback = nullptr;
	allocator=&unsetDefaultAllocator;
}

CloudClient::~CloudClient() = default;

void CloudClient::SetDefaultCallbacks(CloudAllocator *_allocator, CloudClientCallback *_callback)
{
	callback=_callback;
	allocator=_allocator;
}

void CloudClient::Post(CloudKey *cloudKey, const unsigned char *data, uint32_t dataLengthBytes,
                       const RakNetGUID systemIdentifier)
{
	RakAssert(cloudKey);
	
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_POST_REQUEST));
	cloudKey->Serialize(true,&bsOut);
	if (data==nullptr)
		dataLengthBytes=0;
	bsOut.Write(dataLengthBytes);
	if (dataLengthBytes>0)
		bsOut.WriteAlignedBytes((const unsigned char*) data, dataLengthBytes);
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
}

void CloudClient::Release(DataStructures::List<CloudKey> &keys, RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_RELEASE_REQUEST));
	RakAssert(keys.Size() < static_cast<uint16_t>(-1));
	bsOut.WriteCasted<uint16_t>(keys.Size());
	for (uint16_t i = 0; i < keys.Size(); ++i)
	{
		keys[i].Serialize(true,&bsOut);
	}
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
}

bool CloudClient::Get(CloudQuery *keyQuery, RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_GET_REQUEST));
	keyQuery->Serialize(true, &bsOut);
	bsOut.WriteCasted<uint16_t>(0); // Specific systems
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
	return true;
}

bool CloudClient::Get(CloudQuery *keyQuery, DataStructures::List<RakNetGUID> &specificSystems, RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_GET_REQUEST));
	keyQuery->Serialize(true, &bsOut);
	bsOut.WriteCasted<uint16_t>(specificSystems.Size());
	RakAssert(specificSystems.Size() < static_cast<uint16_t>(-1));
	for (uint16_t i=0; i < specificSystems.Size(); ++i)
	{
		bsOut.Write(specificSystems[i]);
	}
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
	return true;
}

bool CloudClient::Get(CloudQuery *cloudQuery, DataStructures::List<CloudQueryRow*> &specificSystems,
                      const RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_GET_REQUEST));
	cloudQuery->Serialize(true, &bsOut);
	bsOut.WriteCasted<uint16_t>(specificSystems.Size());
	RakAssert(specificSystems.Size() < static_cast<uint16_t>(-1));
	for (uint16_t i=0; i < specificSystems.Size(); ++i)
	{
		if (specificSystems[i]->clientGUID!=UNASSIGNED_RAKNET_GUID)
		{
			bsOut.Write(true);
			bsOut.Write(specificSystems[i]->clientGUID);
		}
		else
		{
			bsOut.Write(false);
			bsOut.Write(specificSystems[i]->clientSystemAddress);
		}
	}
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
	return true;
}

void CloudClient::Unsubscribe(DataStructures::List<CloudKey> &keys,
                              const RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_UNSUBSCRIBE_REQUEST));
	RakAssert(keys.Size() < static_cast<uint16_t>(-1));
	bsOut.WriteCasted<uint16_t>(keys.Size());
	for (uint16_t i=0; i < keys.Size(); ++i)
	{
		keys[i].Serialize(true,&bsOut);
	}
	bsOut.WriteCasted<uint16_t>(0);
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
}

void CloudClient::Unsubscribe(DataStructures::List<CloudKey> &keys, DataStructures::List<RakNetGUID> &specificSystems,
                              const RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_UNSUBSCRIBE_REQUEST));
	RakAssert(keys.Size() < static_cast<uint16_t>(-1));
	bsOut.WriteCasted<uint16_t>(keys.Size());
	for (uint16_t i=0; i < keys.Size(); ++i)
	{
		keys[i].Serialize(true,&bsOut);
	}
	bsOut.WriteCasted<uint16_t>(specificSystems.Size());
	RakAssert(specificSystems.Size() < static_cast<uint16_t>(-1));
	for (uint16_t i=0; i < specificSystems.Size(); i++)
	{
		bsOut.Write(specificSystems[i]);
	}
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
}

void CloudClient::Unsubscribe(DataStructures::List<CloudKey> &keys, DataStructures::List<CloudQueryRow*> &specificSystems, RakNetGUID systemIdentifier)
{
	RakNet::BitStream bsOut;
	bsOut.Write(static_cast<MessageID>(ID_CLOUD_UNSUBSCRIBE_REQUEST));
	RakAssert(keys.Size() < static_cast<uint16_t>(-1));
	bsOut.WriteCasted<uint16_t>(keys.Size());
	for (uint16_t i=0; i < keys.Size(); ++i)
	{
		keys[i].Serialize(true,&bsOut);
	}
	bsOut.WriteCasted<uint16_t>(specificSystems.Size());
	RakAssert(specificSystems.Size() < static_cast<uint16_t>(-1));
	for (uint16_t i=0; i < specificSystems.Size(); ++i)
	{
		if (specificSystems[i]->clientGUID!=UNASSIGNED_RAKNET_GUID)
		{
			bsOut.Write(true);
			bsOut.Write(specificSystems[i]->clientGUID);
		}
		else
		{
			bsOut.Write(false);
			bsOut.Write(specificSystems[i]->clientSystemAddress);
		}
	}
	SendUnified(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemIdentifier, false);
}

PluginReceiveResult CloudClient::OnReceive(Packet *packet)
{
	(void) packet;

	return RR_CONTINUE_PROCESSING;
}

[[maybe_unused]] void CloudClient::OnGetReponse(Packet *packet, CloudClientCallback *_callback, CloudAllocator *_allocator)
{
	if (_callback==nullptr)
		_callback=callback;
	if (_allocator==nullptr)
		_allocator=allocator;

	CloudQueryResult cloudQueryResult;

	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	cloudQueryResult.Serialize(false,&bsIn,_allocator);
	bool deallocateRowsAfterReturn=true;
	_callback->OnGet(&cloudQueryResult, &deallocateRowsAfterReturn);
	if (deallocateRowsAfterReturn)
	{
    for (unsigned int i = 0; i < cloudQueryResult.rowsReturned.Size(); ++i)
		{
			_allocator->DeallocateRowData(cloudQueryResult.rowsReturned[i]->data);
			_allocator->DeallocateCloudQueryRow(cloudQueryResult.rowsReturned[i]);
		}
	}
}

[[maybe_unused]] void CloudClient::OnGetReponse(CloudQueryResult *cloudQueryResult, Packet *packet, CloudAllocator *_allocator)
{
	if (_allocator==nullptr)
		_allocator=allocator;

	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	cloudQueryResult->Serialize(false,&bsIn,_allocator);
}

[[maybe_unused]] void CloudClient::OnSubscriptionNotification(Packet *packet, CloudClientCallback *_callback, CloudAllocator *_allocator)
{
	if (_callback==nullptr)
		_callback=callback;
	if (_allocator==nullptr)
		_allocator=allocator;

	bool wasUpdated=false;
	CloudQueryRow row;

	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	bsIn.Read(wasUpdated);
	row.Serialize(false,&bsIn,_allocator);
	bool deallocateRowAfterReturn=true;
	_callback->OnSubscriptionNotification(&row, wasUpdated, &deallocateRowAfterReturn);
	if (deallocateRowAfterReturn)
	{
		_allocator->DeallocateRowData(row.data);
	}
}

[[maybe_unused]] void CloudClient::OnSubscriptionNotification(bool *wasUpdated, CloudQueryRow *row, Packet *packet, CloudAllocator *_allocator)
{
	if (_allocator==nullptr)
		_allocator=allocator;

	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	bool b=false;
	bsIn.Read(b);
	*wasUpdated=b;
	row->Serialize(false,&bsIn,_allocator);
}

void CloudClient::DeallocateWithDefaultAllocator(CloudQueryResult *cloudQueryResult)
{
  for (unsigned int i = 0; i < cloudQueryResult->rowsReturned.Size(); ++i)
	{
		allocator->DeallocateRowData(cloudQueryResult->rowsReturned[i]->data);
		allocator->DeallocateCloudQueryRow(cloudQueryResult->rowsReturned[i]);
	}

	cloudQueryResult->rowsReturned.Clear(false, _FILE_AND_LINE_);
	cloudQueryResult->resultKeyIndices.Clear(false, _FILE_AND_LINE_);
	cloudQueryResult->cloudQuery.keys.Clear(false, _FILE_AND_LINE_);
}

void CloudClient::DeallocateWithDefaultAllocator(CloudQueryRow *row)
{
	allocator->DeallocateRowData(row->data);
}

#endif
