#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <conio.h>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <cstdarg>
