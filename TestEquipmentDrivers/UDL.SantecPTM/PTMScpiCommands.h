#pragma once

// ---------------------------------------------------------------------------
// PTM SCPI constants — User Manual M-PTM_PT1-001.04 Chapter 8 / Tables 7 & 8
// ---------------------------------------------------------------------------

namespace SantecPTM {
namespace SCPI {

// IEEE 488.2 / required
static const char* IDN            = "*IDN?";
static const char* CLS            = "*CLS";
static const char* RST            = "*RST";
static const char* OPC_Q          = "*OPC?";
static const char* SYS_ERR        = "SYST:ERR?";
static const char* SYS_VER        = "SYST:VER?";
static const char* STAT_OPER_COND = "STAT:OPER:COND?";

// Table 8 — PTM commands
static const char* READ_BARCODE   = "READ:BARC?";   // manual: READ:BARCode?
static const char* TEST_NOTIFY    = "TEST:NOTIFY";  // TEST:NOTIFY# "string"
static const char* LCL            = "LCL";
static const char* LCL_Q          = "LCL?";
static const char* LAS_OFF        = "LAS:OFF";
static const char* VFL            = "VFL";          // VFL#
static const char* FVFL           = "FVFL";         // FVFL#
static const char* CHANNEL_Q      = "CHAN?";        // CHANnel?
static const char* DETECT         = "DETECT";       // DETECT#?
static const char* TEST_CON       = "TEST:CON";     // TEST:CON#
static const char* TEST_RESULT_Q  = "TEST:RES?";    // TEST:RESult?
static const char* TEST_START     = "TEST:START";
static const char* TEST_STATUS_Q  = "TEST:STA?";    // TEST:STAtus?
static const char* TEST_NEXT      = "TEST:NEXT";
static const char* TEST_RETRY     = "TEST:RETRY";

} // namespace SCPI
} // namespace SantecPTM
