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
/// \brief \b [Internal] Random number generator
///



#ifndef __RAND_H
#define __RAND_H 

#include "Export.h"

#include <cstdint>

/// Initialise seed for Random Generator
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
/// \param[in] seed The seed value for the random number generator.
extern void RAK_DLL_EXPORT seedMT( uint32_t seed );

/// \internal
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
extern uint32_t RAK_DLL_EXPORT reloadMT( void );

/// Gets a random uint32_t
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
/// \return an integer random value.
extern uint32_t RAK_DLL_EXPORT randomMT( void );

/// Gets a random float
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
/// \return 0 to 1.0f, inclusive
extern float RAK_DLL_EXPORT frandomMT( void );

/// Randomizes a buffer
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
extern void RAK_DLL_EXPORT fillBufferMT( void *buffer, uint32_t bytes );

namespace RakNet {

// Same thing as above functions, but not global
class RAK_DLL_EXPORT RakNetRandom
{
public:
	RakNetRandom();
	~RakNetRandom();
	void SeedMT( uint32_t seed );
	uint32_t ReloadMT( void );
	uint32_t RandomMT( void );
	float FrandomMT( void );
	void FillBufferMT( void *buffer, uint32_t bytes );

protected:
	uint32_t state[ 624 + 1 ];
	uint32_t *next;
	int left;
};

} // namespace RakNet

#endif
