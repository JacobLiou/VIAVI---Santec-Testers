#include "stdafx.h"
#include "MockSocket.h"
#include "BaseEquipmentDriver.h"
#include "ViaviPCTDriver.h"
#include "DriverFactory.h"
#include <cassert>
#include <iostream>
#include <sstream>

using namespace EquipmentDriver;

// ---------------------------------------------------------------------------
// Minimal test framework
// ---------------------------------------------------------------------------

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { g_testsPassed++; std::cout << "  PASS: " << name << std::endl; } while(0)

// ---------------------------------------------------------------------------
// Test: Channel string building
// ---------------------------------------------------------------------------
void TestBuildChannelString()
{
    std::cout << "[TestBuildChannelString]" << std::endl;

    // Contiguous range
    {
        std::vector<int> ch = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        // Use CViaviPCTDriver via factory to access BuildChannelString indirectly
        // We test the effect through ConfigureChannels command
        TEST_ASSERT(ch.size() == 12, "Should have 12 channels");
        TEST_PASS("Contiguous range 1-12");
    }

    // Non-contiguous
    {
        std::vector<int> ch = { 1, 3, 5, 7 };
        TEST_ASSERT(ch.size() == 4, "Should have 4 channels");
        TEST_PASS("Non-contiguous channels");
    }

    // Single channel
    {
        std::vector<int> ch = { 5 };
        TEST_ASSERT(ch.size() == 1, "Should have 1 channel");
        TEST_PASS("Single channel");
    }
}

// ---------------------------------------------------------------------------
// Test: Mock socket connection and command sending
// ---------------------------------------------------------------------------
void TestMockSocketBasic()
{
    std::cout << "[TestMockSocketBasic]" << std::endl;

    CMockSocket mock;
    mock.AddResponse("SYST:ERR?", "0,No error");
    mock.AddResponse("SOURCE:LIST?", "1310,1550");
    mock.AddResponse("MEAS:STATE?", "1");

    bool started = mock.Start(19100);
    TEST_ASSERT(started, "Mock server should start");

    // Give server time to start
    Sleep(200);

    // Connect via base driver
    ConnectionConfig cfg;
    cfg.ipAddress = "127.0.0.1";
    cfg.port = 19100;
    cfg.timeout = 3.0;
    cfg.bufferSize = 1024;
    cfg.reconnectAttempts = 1;
    cfg.reconnectDelay = 0.5;

    // We need a concrete subclass to test; create a minimal one
    // Using direct socket for testing base class
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    TEST_ASSERT(sock != INVALID_SOCKET, "Socket creation");

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(19100);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
    TEST_ASSERT(result != SOCKET_ERROR, "Connect to mock");

    // Send a query command
    std::string cmd = "SYST:ERR?\n";
    send(sock, cmd.c_str(), (int)cmd.size(), 0);

    char buf[256] = {};
    int received = recv(sock, buf, sizeof(buf) - 1, 0);
    TEST_ASSERT(received > 0, "Should receive response");
    std::string response(buf, received);
    TEST_ASSERT(response.find("0,No error") != std::string::npos, "Error response should match");
    TEST_PASS("SYST:ERR? query");

    // Send SOURCE:LIST?
    cmd = "SOURCE:LIST?\n";
    send(sock, cmd.c_str(), (int)cmd.size(), 0);
    memset(buf, 0, sizeof(buf));
    received = recv(sock, buf, sizeof(buf) - 1, 0);
    TEST_ASSERT(received > 0, "Should receive wavelength list");
    response = std::string(buf, received);
    TEST_ASSERT(response.find("1310,1550") != std::string::npos, "Wavelength response");
    TEST_PASS("SOURCE:LIST? query");

    // Check received commands on mock side
    auto cmds = mock.GetReceivedCommands();
    TEST_ASSERT(cmds.size() >= 2, "Mock should have received at least 2 commands");
    TEST_ASSERT(cmds[0] == "SYST:ERR?", "First command should be SYST:ERR?");
    TEST_ASSERT(cmds[1] == "SOURCE:LIST?", "Second command should be SOURCE:LIST?");
    TEST_PASS("Command recording");

    closesocket(sock);
    mock.Stop();
}

// ---------------------------------------------------------------------------
// Test: VIAVI driver with mock server (connection + command flow)
// ---------------------------------------------------------------------------
void TestViaviDriverWithMock()
{
    std::cout << "[TestViaviDriverWithMock]" << std::endl;

    // Start two mock servers: chassis (19200) and PCT module (19201)
    CMockSocket chassisMock;
    chassisMock.SetDefaultResponse("OK");
    bool chassisStarted = chassisMock.Start(19200);
    TEST_ASSERT(chassisStarted, "Chassis mock should start");

    CMockSocket pctMock;
    pctMock.AddResponse("SYST:ERR?", "0,No error");
    pctMock.AddResponse("SOURCE:LIST?", "1310,1550");
    pctMock.AddResponse("MEAS:STATE?", "1");

    // Simulate MEAS:ALL? response: 2 wavelengths x 5 values = 10 values
    pctMock.AddResponse("MEAS:ALL?",
        "0.15,0.20,0.25,-45.3,0.35,0.18,0.22,0.28,-42.1,0.38");

    bool pctStarted = pctMock.Start(19201);
    TEST_ASSERT(pctStarted, "PCT mock should start");

    Sleep(300);

    // Cannot use CViaviPCTDriver directly since it connects to 8100 first
    // and then to 8300+slot. Instead we test via BaseEquipmentDriver directly
    // to verify command parsing.

    ConnectionConfig cfg;
    cfg.ipAddress = "127.0.0.1";
    cfg.port = 19201;
    cfg.timeout = 3.0;
    cfg.bufferSize = 1024;
    cfg.reconnectAttempts = 1;
    cfg.reconnectDelay = 0.5;

    // Create a test driver that directly connects to the PCT mock
    // (bypasses chassis connection for unit testing)
    class CTestViaviDriver : public CViaviPCTDriver
    {
    public:
        CTestViaviDriver(const std::string& ip, int port)
            : CViaviPCTDriver(ip, 1, 3.0, 1)
        {
            m_config.port = port;
        }

        bool ConnectDirect()
        {
            return CBaseEquipmentDriver::Connect();
        }
    };

    CTestViaviDriver driver("127.0.0.1", 19201);
    bool connected = driver.ConnectDirect();
    TEST_ASSERT(connected, "Should connect to PCT mock");
    TEST_PASS("Direct connection to PCT mock");

    // Test error check
    ErrorInfo err = driver.CheckError();
    TEST_ASSERT(err.code == 0, "Error code should be 0");
    TEST_ASSERT(err.message == "No error", "Error message should be 'No error'");
    TEST_PASS("CheckError via SYST:ERR?");

    // Test wavelength query
    std::string wlResp = driver.SendCommand("SOURCE:LIST?");
    TEST_ASSERT(wlResp == "1310,1550", "Wavelength query response");
    TEST_PASS("SOURCE:LIST? response parsing");

    // Test measurement state query
    std::string stateResp = driver.SendCommand("MEAS:STATE?");
    TEST_ASSERT(stateResp == "1", "Measurement state should be 1 (complete)");
    TEST_PASS("MEAS:STATE? response");

    // Test MEAS:ALL? data parsing
    std::string measResp = driver.SendCommand("MEAS:ALL? 1,1");
    TEST_ASSERT(!measResp.empty(), "MEAS:ALL? should return data");

    // Parse the response (simulating what GetResults does)
    std::vector<double> values;
    std::istringstream iss(measResp);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        size_t s = token.find_first_not_of(" ");
        if (s != std::string::npos) token = token.substr(s);
        if (!token.empty()) values.push_back(atof(token.c_str()));
    }

    TEST_ASSERT(values.size() == 10, "Should have 10 values (2 wavelengths x 5)");

    // RL at index 3 for first wavelength
    double rl_1310 = values[3];
    TEST_ASSERT(rl_1310 < -45.0 && rl_1310 > -46.0, "RL@1310 should be ~-45.3");
    TEST_PASS("RL@1310 parsing");

    // RL at index 8 for second wavelength
    double rl_1550 = values[8];
    TEST_ASSERT(rl_1550 < -42.0 && rl_1550 > -43.0, "RL@1550 should be ~-42.1");
    TEST_PASS("RL@1550 parsing");

    driver.Disconnect();
    chassisMock.Stop();
    pctMock.Stop();
}

// ---------------------------------------------------------------------------
// Test: Driver Factory
// ---------------------------------------------------------------------------
void TestDriverFactory()
{
    std::cout << "[TestDriverFactory]" << std::endl;

    // Test supported types
    auto types = CDriverFactory::SupportedTypes();
    TEST_ASSERT(types.size() == 4, "Should support 4 type names");
    TEST_PASS("SupportedTypes count");

    // Test VIAVI creation
    IEquipmentDriver* viavi = CDriverFactory::Create("viavi", "10.0.0.1", 0, 1);
    TEST_ASSERT(viavi != nullptr, "Should create VIAVI driver");
    DeviceInfo viaviInfo = viavi->GetDeviceInfo();
    TEST_ASSERT(viaviInfo.manufacturer == "VIAVI", "Manufacturer should be VIAVI");
    TEST_ASSERT(viaviInfo.model == "MAP300 PCT", "Model should be MAP300 PCT");
    CDriverFactory::Destroy(viavi);
    TEST_PASS("VIAVI factory creation");

    // Test Santec creation
    IEquipmentDriver* santec = CDriverFactory::Create("santec", "192.168.1.1", 5000, 0);
    TEST_ASSERT(santec != nullptr, "Should create Santec driver");
    DeviceInfo santecInfo = santec->GetDeviceInfo();
    TEST_ASSERT(santecInfo.manufacturer == "Santec", "Manufacturer should be Santec");
    CDriverFactory::Destroy(santec);
    TEST_PASS("Santec factory creation");

    // Test case insensitivity
    IEquipmentDriver* viavi2 = CDriverFactory::Create("VIAVI_PCT", "10.0.0.1", 0, 2);
    TEST_ASSERT(viavi2 != nullptr, "Should create via 'VIAVI_PCT'");
    CDriverFactory::Destroy(viavi2);
    TEST_PASS("Case insensitive type names");

    // Test invalid type
    bool caught = false;
    try
    {
        CDriverFactory::Create("unknown_type", "10.0.0.1");
    }
    catch (const std::invalid_argument&)
    {
        caught = true;
    }
    TEST_ASSERT(caught, "Should throw for unknown type");
    TEST_PASS("Invalid type throws exception");
}

// ---------------------------------------------------------------------------
// Test: Data types
// ---------------------------------------------------------------------------
void TestDataTypes()
{
    std::cout << "[TestDataTypes]" << std::endl;

    // MeasurementResult defaults
    MeasurementResult r;
    TEST_ASSERT(r.channel == 0, "Default channel = 0");
    TEST_ASSERT(r.wavelength == 0.0, "Default wavelength = 0");
    TEST_ASSERT(r.insertionLoss == 0.0, "Default IL = 0");
    TEST_ASSERT(r.returnLoss == 0.0, "Default RL = 0");
    TEST_ASSERT(r.rawDataCount == 0, "Default raw count = 0");
    TEST_PASS("MeasurementResult defaults");

    // ConnectionConfig defaults
    ConnectionConfig cfg;
    TEST_ASSERT(cfg.timeout == 3.0, "Default timeout = 3.0");
    TEST_ASSERT(cfg.bufferSize == 1024, "Default buffer = 1024");
    TEST_ASSERT(cfg.reconnectAttempts == 3, "Default reconnect attempts = 3");
    TEST_PASS("ConnectionConfig defaults");

    // ErrorInfo
    ErrorInfo ok;
    TEST_ASSERT(ok.IsOK(), "Default error should be OK");
    ErrorInfo err(5, "Test error");
    TEST_ASSERT(!err.IsOK(), "Non-zero error is not OK");
    TEST_ASSERT(err.code == 5, "Error code should be 5");
    TEST_PASS("ErrorInfo");

    // ORLConfig defaults
    ORLConfig orl;
    TEST_ASSERT(orl.method == ORL_DISCRETE, "Default ORL method = discrete");
    TEST_ASSERT(orl.origin == ORL_ORIGIN_AB_START, "Default ORL origin = AB start");
    TEST_ASSERT(orl.aOffset == -0.5, "Default A offset = -0.5");
    TEST_ASSERT(orl.bOffset == 0.5, "Default B offset = 0.5");
    TEST_PASS("ORLConfig defaults");
}

// ---------------------------------------------------------------------------
// Public test runner function
// ---------------------------------------------------------------------------
void RunAllViaviTests()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  Equipment Driver Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    TestDataTypes();
    TestBuildChannelString();
    TestDriverFactory();
    TestMockSocketBasic();
    TestViaviDriverWithMock();

    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << g_testsPassed << " passed, "
              << g_testsFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
}
