/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "../include/RakNet/FormatString.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "../include/RakNet/LinuxStrings.h"

char * FormatString(const char *format, ...)
{
	static int textIndex=0;
	static char text[4][8096];
	va_list ap;
	va_start(ap, format);

	if (++textIndex == 4)
		textIndex = 0;

	int written = std::vsnprintf(text[textIndex],
	                             sizeof(text[textIndex]),
	                             format,
	                             ap);
	va_end(ap);

	if (written < 0 ||
	    written >= static_cast<int>(sizeof(text[textIndex])))
	{
		text[textIndex][sizeof(text[textIndex]) - 1] = '\0';
	}
	
	return text[textIndex];
}

char * FormatStringTS(char *output, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int written = std::vsnprintf(output, 512, format, ap);
	va_end(ap);

	if (written < 0 || written >= 512)
		output[511] = '\0';

	return output;
}

