#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")
