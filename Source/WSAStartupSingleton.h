/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifndef __WSA_STARTUP_SINGLETON_H
#define __WSA_STARTUP_SINGLETON_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <mutex>
#include <stdexcept>
#include <string>

class WSAStartupSingleton
{
	WSAStartupSingleton();
	~WSAStartupSingleton();

	static inline int refCount = 0;
	static inline std::mutex refMutex;
#ifdef _WIN32
	WSADATA wsaData{};
#endif

public:
	static WSAStartupSingleton& instance();

	WSAStartupSingleton(const WSAStartupSingleton&) = delete;
	WSAStartupSingleton& operator=(const WSAStartupSingleton&) = delete;
	WSAStartupSingleton(WSAStartupSingleton&&) = delete;
	WSAStartupSingleton& operator=(WSAStartupSingleton&&) = delete;

	void addRef();
	void release();
};

#endif
