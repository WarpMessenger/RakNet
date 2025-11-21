/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#if defined(_WIN32)

#include <conio.h> // getche()

#elif defined(__S3E__)

#else

#include "../include/RakNet/Getche.h"
#include <termios.h>
#include <unistd.h>
#include <cstdio>

char getche()
{
    termios oldt{};
    termios newt{};

    if (tcgetattr(STDIN_FILENO, &oldt) == -1)
        return '\0';

    newt = oldt;
    newt.c_lflag &= static_cast<unsigned>(~(ICANON | ECHO));

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1)
        return '\0';

    int ch = std::getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (ch == EOF)
        return '\0';

    return static_cast<char>(ch);
}

#endif
