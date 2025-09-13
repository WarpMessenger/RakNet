/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "WSAStartupSingleton.h"

#include "RakNetDefines.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <format>
#include <iostream>

WSAStartupSingleton& WSAStartupSingleton::instance(){
	static WSAStartupSingleton singleton;
	return singleton;
}

WSAStartupSingleton::WSAStartupSingleton() {}

WSAStartupSingleton::~WSAStartupSingleton() {
	std::scoped_lock lock(refMutex);
	if(refCount > 0){
		#ifdef _WIN32
		WSACleanup();
		#endif
		refCount = 0;
	}
}

void WSAStartupSingleton::addRef()
{
#ifdef _WIN32
	std::scoped_lock lock(refMutex);
	if (++refCount == 1) {
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			DWORD error = GetLastError();
			throw std::runtime_error(
				std::format("WSAStartup failed with error code: {}", error)
			);
		}
	}
#endif
}

void WSAStartupSingleton::release()
{
#if defined(_WIN32) && !defined(WINDOWS_STORE_RT)
	std::scoped_lock lock(refMutex);

	if (refCount == 0) {
		throw std::logic_error("WSAStartupSingleton::release called without matching addRef");
	}

	if (--refCount == 0) {
		WSACleanup();
	}
#endif
}
