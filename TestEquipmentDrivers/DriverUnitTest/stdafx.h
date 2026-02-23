#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <iostream>
#include <functional>
