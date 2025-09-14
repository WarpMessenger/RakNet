/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/// \file
///



#include "RakNetStatistics.h"
#include <cstdio>
#include "GetTime.h"
#include "RakString.h"

using namespace RakNet;

// Verbosity level currently supports 0 (low), 1 (medium), 2 (high)
// Buffer must be hold enough to hold the output string.  See the source to get an idea of how many bytes will be output
void RAK_DLL_EXPORT RakNet::StatisticsToString(RakNetStatistics* s, char* buffer, int verbosityLevel)
{
    if (s == nullptr)
    {
        std::snprintf(buffer, 512, "stats is a NULL pointer in statsToString\n");
        return;
    }

    buffer[0] = '\0';
    size_t bufferSize = 8192;
    size_t offset = 0;

    auto append = [&](const char* fmt, auto... args)
    {
        if (offset >= bufferSize)
            return;
        int written = std::snprintf(buffer + offset, bufferSize - offset, fmt, args...);
        if (written > 0)
        {
            offset += static_cast<size_t>(written);
            if (offset > bufferSize)
                offset = bufferSize; // на всякий случай
        }
    };

    if (verbosityLevel == 0)
    {
        append("Bytes per second sent     %" PRINTF_64_BIT_MODIFIER "u\n"
               "Bytes per second received %" PRINTF_64_BIT_MODIFIER "u\n"
               "Current packetloss        %.1f%%\n",
               (unsigned long long)s->valueOverLastSecond[ACTUAL_BYTES_SENT],
               (unsigned long long)s->valueOverLastSecond[ACTUAL_BYTES_RECEIVED],
               s->packetlossLastSecond * 100.0f);
    }
    else if (verbosityLevel == 1)
    {
        append("Actual bytes per second sent       %" PRINTF_64_BIT_MODIFIER "u\n"
               "Actual bytes per second received   %" PRINTF_64_BIT_MODIFIER "u\n"
               "Message bytes per second pushed    %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total actual bytes sent            %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total actual bytes received        %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total message bytes pushed         %" PRINTF_64_BIT_MODIFIER "u\n"
               "Current packetloss                 %.1f%%\n"
               "Average packetloss                 %.1f%%\n"
               "Elapsed connection time in seconds %" PRINTF_64_BIT_MODIFIER "u\n",
               (unsigned long long)s->valueOverLastSecond[ACTUAL_BYTES_SENT],
               (unsigned long long)s->valueOverLastSecond[ACTUAL_BYTES_RECEIVED],
               (unsigned long long)s->valueOverLastSecond[USER_MESSAGE_BYTES_PUSHED],
               (unsigned long long)s->runningTotal[ACTUAL_BYTES_SENT],
               (unsigned long long)s->runningTotal[ACTUAL_BYTES_RECEIVED],
               (unsigned long long)s->runningTotal[USER_MESSAGE_BYTES_PUSHED],
               s->packetlossLastSecond * 100.0f,
               s->packetlossTotal * 100.0f,
               (unsigned long long)((RakNet::GetTimeUS() - s->connectionStartTime) / 1000000ULL));

        if (s->BPSLimitByCongestionControl != 0)
        {
            append("Send capacity                    %" PRINTF_64_BIT_MODIFIER "u bytes per second (%.0f%%)\n",
                   (unsigned long long)s->BPSLimitByCongestionControl,
                   100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByCongestionControl);
        }
        if (s->BPSLimitByOutgoingBandwidthLimit != 0)
        {
            append("Send limit                       %" PRINTF_64_BIT_MODIFIER "u (%.0f%%)\n",
                   (unsigned long long)s->BPSLimitByOutgoingBandwidthLimit,
                   100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByOutgoingBandwidthLimit);
        }
    }
    else // verbosityLevel >= 2
    {
        append("Actual bytes per second sent         %" PRINTF_64_BIT_MODIFIER "u\n"
               "Actual bytes per second received     %" PRINTF_64_BIT_MODIFIER "u\n"
               "Message bytes per second sent        %" PRINTF_64_BIT_MODIFIER "u\n"
               "Message bytes per second resent      %" PRINTF_64_BIT_MODIFIER "u\n"
               "Message bytes per second pushed      %" PRINTF_64_BIT_MODIFIER "u\n"
               "Message bytes per second returned    %" PRINTF_64_BIT_MODIFIER "u\n"
               "Message bytes per second ignored     %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total bytes sent                     %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total bytes received                 %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total message bytes sent             %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total message bytes resent           %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total message bytes pushed           %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total message bytes returned         %" PRINTF_64_BIT_MODIFIER "u\n"
               "Total message bytes ignored          %" PRINTF_64_BIT_MODIFIER "u\n"
               "Messages in send buffer, by priority %i,%i,%i,%i\n"
               "Bytes in send buffer, by priority    %i,%i,%i,%i\n"
               "Messages in resend buffer            %i\n"
               "Bytes in resend buffer               %" PRINTF_64_BIT_MODIFIER "u\n"
               "Current packetloss                   %.1f%%\n"
               "Average packetloss                   %.1f%%\n"
               "Elapsed connection time in seconds   %" PRINTF_64_BIT_MODIFIER "u\n",
               (unsigned long long)s->valueOverLastSecond[ACTUAL_BYTES_SENT],
               (unsigned long long)s->valueOverLastSecond[ACTUAL_BYTES_RECEIVED],
               (unsigned long long)s->valueOverLastSecond[USER_MESSAGE_BYTES_SENT],
               (unsigned long long)s->valueOverLastSecond[USER_MESSAGE_BYTES_RESENT],
               (unsigned long long)s->valueOverLastSecond[USER_MESSAGE_BYTES_PUSHED],
               (unsigned long long)s->valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_PROCESSED],
               (unsigned long long)s->valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_IGNORED],
               (unsigned long long)s->runningTotal[ACTUAL_BYTES_SENT],
               (unsigned long long)s->runningTotal[ACTUAL_BYTES_RECEIVED],
               (unsigned long long)s->runningTotal[USER_MESSAGE_BYTES_SENT],
               (unsigned long long)s->runningTotal[USER_MESSAGE_BYTES_RESENT],
               (unsigned long long)s->runningTotal[USER_MESSAGE_BYTES_PUSHED],
               (unsigned long long)s->runningTotal[USER_MESSAGE_BYTES_RECEIVED_PROCESSED],
               (unsigned long long)s->runningTotal[USER_MESSAGE_BYTES_RECEIVED_IGNORED],
               s->messageInSendBuffer[IMMEDIATE_PRIORITY],
               s->messageInSendBuffer[HIGH_PRIORITY],
               s->messageInSendBuffer[MEDIUM_PRIORITY],
               s->messageInSendBuffer[LOW_PRIORITY],
               (unsigned int)s->bytesInSendBuffer[IMMEDIATE_PRIORITY],
               (unsigned int)s->bytesInSendBuffer[HIGH_PRIORITY],
               (unsigned int)s->bytesInSendBuffer[MEDIUM_PRIORITY],
               (unsigned int)s->bytesInSendBuffer[LOW_PRIORITY],
               s->messagesInResendBuffer,
               (unsigned long long)s->bytesInResendBuffer,
               s->packetlossLastSecond * 100.0f,
               s->packetlossTotal * 100.0f,
               (unsigned long long)((RakNet::GetTimeUS() - s->connectionStartTime) / 1000000ULL));

        if (s->BPSLimitByCongestionControl != 0)
        {
            append("Send capacity                    %" PRINTF_64_BIT_MODIFIER "u bytes per second (%.0f%%)\n",
                   (unsigned long long)s->BPSLimitByCongestionControl,
                   100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByCongestionControl);
        }
        if (s->BPSLimitByOutgoingBandwidthLimit != 0)
        {
            append("Send limit                       %" PRINTF_64_BIT_MODIFIER "u (%.0f%%)\n",
                   (unsigned long long)s->BPSLimitByOutgoingBandwidthLimit,
                   100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByOutgoingBandwidthLimit);
        }
    }
}
