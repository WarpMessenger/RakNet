/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <RakNet/CCRakNetSlidingWindow.h>
#include <RakNet/MTUSize.h>

#if USE_SLIDING_WINDOW_CONGESTION_CONTROL == 1

static constexpr double UNSET_TIME_US=-1;

#if CC_TIME_TYPE_BYTES==4
static const CCTimeType SYN=10;
#else
static constexpr CCTimeType SYN=10000;
#endif

#include <RakNet/RakAssert.h>

using namespace RakNet;

// ****************************************************** PUBLIC METHODS ******************************************************

CCRakNetSlidingWindow::CCRakNetSlidingWindow()
{

}
// ----------------------------------------------------------------------------------------------------------------------------
CCRakNetSlidingWindow::~CCRakNetSlidingWindow()
{

}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::Init(const CCTimeType curTime,
                                 const uint32_t maxDatagramPayload)
{
    (void) curTime;

    lastRtt=estimatedRTT=deviationRtt=UNSET_TIME_US;
    RakAssert(maxDatagramPayload <= MAXIMUM_MTU_SIZE);
    MAXIMUM_MTU_INCLUDING_UDP_HEADER=maxDatagramPayload;
    cwnd=maxDatagramPayload;
    ssThresh=0.0;
    oldestUnsentAck=0;
    nextDatagramSequenceNumber=0;
    nextCongestionControlBlock=0;
    backoffThisBlock=speedUpThisBlock=false;
    expectedNextSequenceNumber=0;
    _isContinuousSend=false;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::Update(const CCTimeType curTime,
                                   const bool hasDataToSendOrResend)
{
    (void) curTime;
    (void) hasDataToSendOrResend;
}
// ----------------------------------------------------------------------------------------------------------------------------
int CCRakNetSlidingWindow::GetRetransmissionBandwidth(
    const CCTimeType curTime, const CCTimeType timeSinceLastTick,
    const uint32_t unacknowledgedBytes, const bool isContinuousSend)
{
    (void) curTime;
    (void) isContinuousSend;
    (void) timeSinceLastTick;

    return unacknowledgedBytes;
}
// ----------------------------------------------------------------------------------------------------------------------------
int CCRakNetSlidingWindow::GetTransmissionBandwidth(
    const CCTimeType curTime, const CCTimeType timeSinceLastTick,
    const uint32_t unacknowledgedBytes, const bool isContinuousSend)
{
	(void) curTime;
	(void) timeSinceLastTick;

	_isContinuousSend=isContinuousSend;

	if (unacknowledgedBytes<=cwnd)
		return static_cast<int>(cwnd - unacknowledgedBytes);
	else
		return 0;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetSlidingWindow::ShouldSendACKs(const CCTimeType curTime, const CCTimeType estimatedTimeToNextTick)
{
  const CCTimeType rto = GetSenderRTOForACK();
	(void) estimatedTimeToNextTick;

	// iphone crashes on comparison between double and int64 http://www.jenkinssoftware.com/forum/index.php?topic=2717.0
	if (rto== static_cast<CCTimeType>(UNSET_TIME_US))
	{
		// Unknown how long until the remote system will retransmit, so better send right away
		return true;
	}

	return curTime >= oldestUnsentAck + SYN;
}
// ----------------------------------------------------------------------------------------------------------------------------
DatagramSequenceNumberType CCRakNetSlidingWindow::GetNextDatagramSequenceNumber()
{
	return nextDatagramSequenceNumber;
}
// ----------------------------------------------------------------------------------------------------------------------------
DatagramSequenceNumberType CCRakNetSlidingWindow::GetAndIncrementNextDatagramSequenceNumber()
{
	DatagramSequenceNumberType dsnt=nextDatagramSequenceNumber;
	++nextDatagramSequenceNumber;
	return dsnt;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnSendBytes(const CCTimeType curTime,
                                        const uint32_t numBytes)
{
	(void) curTime;
	(void) numBytes;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnGotPacketPair(
    const DatagramSequenceNumberType& datagramSequenceNumber,
    const uint32_t sizeInBytes, const CCTimeType curTime)
{
	(void) curTime;
	(void) sizeInBytes;
	(void) datagramSequenceNumber;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetSlidingWindow::OnGotPacket(
    const DatagramSequenceNumberType& datagramSequenceNumber,
    const bool isContinuousSend, const CCTimeType curTime,
    const uint32_t sizeInBytes, uint32_t *skippedMessageCount)
{
	(void) curTime;
	(void) sizeInBytes;
	(void) isContinuousSend;

	if (oldestUnsentAck==0)
		oldestUnsentAck=curTime;

	if (datagramSequenceNumber==expectedNextSequenceNumber)
	{
		*skippedMessageCount=0;
		expectedNextSequenceNumber=datagramSequenceNumber+ static_cast<DatagramSequenceNumberType>(1);
	}
	else if (GreaterThan(datagramSequenceNumber, expectedNextSequenceNumber))
	{
		*skippedMessageCount=datagramSequenceNumber-expectedNextSequenceNumber;
		// Sanity check, just use timeout resend if this was really valid
		if (*skippedMessageCount>1000)
		{
			// During testing, the nat punchthrough server got 51200 on the first packet. I have no idea where this comes from, but has happened twice
			if (*skippedMessageCount> static_cast<uint32_t>(50000))
				return false;
			*skippedMessageCount=1000;
		}
		expectedNextSequenceNumber=datagramSequenceNumber+ static_cast<DatagramSequenceNumberType>(1);
	}
	else
	{
		*skippedMessageCount=0;
	}

	return true;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnResend(const CCTimeType curTime,
                                     const RakNet::TimeUS nextActionTime)
{
	(void) curTime;
	(void) nextActionTime;

	if (_isContinuousSend && backoffThisBlock==false && cwnd>MAXIMUM_MTU_INCLUDING_UDP_HEADER*2)
	{
		// Spec says 1/2 cwnd, but it never recovers because cwnd increases too slowly
		//ssThresh=cwnd-8.0 * (MAXIMUM_MTU_INCLUDING_UDP_HEADER*MAXIMUM_MTU_INCLUDING_UDP_HEADER/cwnd);
		ssThresh=cwnd/2;
		if (ssThresh<MAXIMUM_MTU_INCLUDING_UDP_HEADER)
			ssThresh=MAXIMUM_MTU_INCLUDING_UDP_HEADER;
		cwnd=MAXIMUM_MTU_INCLUDING_UDP_HEADER;

		// Only backoff once per period
		nextCongestionControlBlock=nextDatagramSequenceNumber;
		backoffThisBlock=true;

		// CC PRINTF
		//printf("-- %.0f (Resend) Enter slow start.\n", cwnd);
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnNAK(
    const CCTimeType curTime, const DatagramSequenceNumberType& nakSequenceNumber)
{
	(void) nakSequenceNumber;
	(void) curTime;

	if (_isContinuousSend && backoffThisBlock==false)
	{
		// Start congestion avoidance
		ssThresh=cwnd/2;

		// CC PRINTF
		//printf("- %.0f (NAK) Set congestion avoidance.\n", cwnd);
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnAck(const CCTimeType curTime,
                                  const CCTimeType rtt, const bool hasBAndAS,
                                  const BytesPerMicrosecond B,
                                  const BytesPerMicrosecond AS,
                                  const double totalUserDataBytesAcked,
                                  const bool isContinuousSend,
    const DatagramSequenceNumberType& sequenceNumber )
{
	(void) B;
	(void) totalUserDataBytesAcked;
	(void) AS;
	(void) hasBAndAS;
	(void) curTime;
	(void) rtt;

	lastRtt= static_cast<double>(rtt);
	if (estimatedRTT==UNSET_TIME_US)
	{
		estimatedRTT= static_cast<double>(rtt);
		deviationRtt= static_cast<double>(rtt);
	}
	else
	{
    const double d = .05;
    const double difference = rtt - estimatedRTT;
		estimatedRTT = estimatedRTT + d * difference;
		deviationRtt = deviationRtt + d * (abs(difference) - deviationRtt);
	}

	_isContinuousSend=isContinuousSend;

	if (isContinuousSend == false)
    return;

  const bool isNewCongestionControlPeriod =
      GreaterThan(sequenceNumber, nextCongestionControlBlock);

	if (isNewCongestionControlPeriod)
	{
		backoffThisBlock=false;
		speedUpThisBlock=false;
		nextCongestionControlBlock=nextDatagramSequenceNumber;
	}

	if (IsInSlowStart())
	{
		cwnd+=MAXIMUM_MTU_INCLUDING_UDP_HEADER;
		if (cwnd > ssThresh && ssThresh!=0)
			cwnd = ssThresh + MAXIMUM_MTU_INCLUDING_UDP_HEADER*MAXIMUM_MTU_INCLUDING_UDP_HEADER/cwnd;

		// CC PRINTF
	//	printf("++ %.0f Slow start increase.\n", cwnd);

	}
	else if (isNewCongestionControlPeriod)
	{
		cwnd+=MAXIMUM_MTU_INCLUDING_UDP_HEADER*MAXIMUM_MTU_INCLUDING_UDP_HEADER/cwnd;

		// CC PRINTF
		// printf("+ %.0f Congestion avoidance increase.\n", cwnd);
	}
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnDuplicateAck( CCTimeType curTime, const DatagramSequenceNumberType& sequenceNumber )
{
	(void) curTime;
	(void) sequenceNumber;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnSendAckGetBAndAS(const CCTimeType curTime, bool *hasBAndAS,
                                               const BytesPerMicrosecond *B,
                                               const BytesPerMicrosecond *AS)
{
	(void) curTime;
	(void) B;
	(void) AS;

	*hasBAndAS=false;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnSendAck(const CCTimeType curTime,
                                      const uint32_t numBytes)
{
	(void) curTime;
	(void) numBytes;

	oldestUnsentAck=0;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::OnSendNACK(const CCTimeType curTime,
                                       const uint32_t numBytes)
{
	(void) curTime;
	(void) numBytes;

}
// ----------------------------------------------------------------------------------------------------------------------------
CCTimeType CCRakNetSlidingWindow::GetRTOForRetransmission(
    const unsigned char timesSent) const
{
	(void) timesSent;

#if CC_TIME_TYPE_BYTES==4
	const CCTimeType maxThreshold=2000;
	//const CCTimeType minThreshold=100;
	const CCTimeType additionalVariance=30;
#else
  constexpr CCTimeType maxThreshold=2000000;
	// const CCTimeType minThreshold=100000;
  constexpr CCTimeType additionalVariance=30000;
#endif


	if (estimatedRTT==UNSET_TIME_US)
		return maxThreshold;

 	//double u=1.0f;
	double u=2.0f;
 	double q = 4.0f;

  const CCTimeType threshhold = static_cast<CCTimeType>(u * estimatedRTT + q * deviationRtt) + additionalVariance;
	if (threshhold > maxThreshold)
		return maxThreshold;
	return threshhold;
}
// ----------------------------------------------------------------------------------------------------------------------------
void CCRakNetSlidingWindow::SetMTU(const uint32_t bytes)
{
	RakAssert(bytes < MAXIMUM_MTU_SIZE);
	MAXIMUM_MTU_INCLUDING_UDP_HEADER=bytes;
}
// ----------------------------------------------------------------------------------------------------------------------------
uint32_t CCRakNetSlidingWindow::GetMTU() const
{
	return MAXIMUM_MTU_INCLUDING_UDP_HEADER;
}
// ----------------------------------------------------------------------------------------------------------------------------
BytesPerMicrosecond CCRakNetSlidingWindow::GetLocalReceiveRate(
    const CCTimeType currentTime) const
{
	(void) currentTime;

	return 0; // TODO
}
// ----------------------------------------------------------------------------------------------------------------------------
double CCRakNetSlidingWindow::GetRTT() const
{
	if (lastRtt==UNSET_TIME_US)
		return 0.0;
	return lastRtt;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetSlidingWindow::GreaterThan(const DatagramSequenceNumberType& a,
                                        const DatagramSequenceNumberType& b)
{
	// a > b?
	const DatagramSequenceNumberType halfSpan = (static_cast<DatagramSequenceNumberType>(
                                       static_cast<const uint32_t>(-1)) /
       static_cast<DatagramSequenceNumberType>(2));
	return b!=a && b-a>halfSpan;
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetSlidingWindow::LessThan(const DatagramSequenceNumberType& a,
                                     const DatagramSequenceNumberType& b)
{
	// a < b?
	const DatagramSequenceNumberType halfSpan = static_cast<DatagramSequenceNumberType>(static_cast<const uint32_t>(-1)) /
      static_cast<DatagramSequenceNumberType>(2);
	return b!=a && b-a<halfSpan;
}
// ----------------------------------------------------------------------------------------------------------------------------
uint64_t CCRakNetSlidingWindow::GetBytesPerSecondLimitByCongestionControl() const
{
	return 0; // TODO
}
// ----------------------------------------------------------------------------------------------------------------------------
CCTimeType CCRakNetSlidingWindow::GetSenderRTOForACK() const
{
	if (lastRtt==UNSET_TIME_US)
		return static_cast<CCTimeType>(UNSET_TIME_US);
	return static_cast<CCTimeType>(lastRtt + SYN);
}
// ----------------------------------------------------------------------------------------------------------------------------
bool CCRakNetSlidingWindow::IsInSlowStart() const
{
	return cwnd <= ssThresh || ssThresh==0;
}
// ----------------------------------------------------------------------------------------------------------------------------
#endif
