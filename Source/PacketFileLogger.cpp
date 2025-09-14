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
#if _RAKNET_SUPPORT_PacketLogger == 1
#include <RakNet/PacketFileLogger.h>
#include <RakNet/GetTime.h>
#include <format>
#include <string>
#include <cstring>

using namespace RakNet;

PacketFileLogger::PacketFileLogger()
{
	packetLogFile=0;
}
PacketFileLogger::~PacketFileLogger()
{
	if (packetLogFile)
	{
		fflush(packetLogFile);
		fclose(packetLogFile);
	}
}
void PacketFileLogger::StartLog(const char *filenamePrefix)
{
    // Open file for writing
    std::string filename;
    if (strlen(filenamePrefix) > 0)
        filename = std::format("{}_{}.csv", filenamePrefix, static_cast<int>(RakNet::GetTimeMS()));
    else
        filename = std::format("PacketLog_{}.csv", static_cast<int>(RakNet::GetTimeMS()));

    packetLogFile = fopen(filename.c_str(), "wt");
    LogHeader();
    if (packetLogFile)
    {
        fflush(packetLogFile);
    }
}

void PacketFileLogger::WriteLog(const char *str)
{
	if (packetLogFile)
	{
		fprintf(packetLogFile, "%s\n", str);
		fflush(packetLogFile);
	}
}

#endif // _RAKNET_SUPPORT_*
