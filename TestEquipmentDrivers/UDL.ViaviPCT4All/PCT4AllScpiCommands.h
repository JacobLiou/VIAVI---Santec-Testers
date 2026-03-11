#pragma once

// ===========================================================================
// Viavi MAP PCT SCPI 命令常量定义
//
// 严格对照文档: MAP-PCT Programming Guide 22112369-346 R002
// 所有命令字符串按文档章节组织
// ===========================================================================

namespace ViaviPCT4All {
namespace SCPI {

// ---- Appendix A: Common SCPI Commands ----
namespace Common {
    static const char* CLS             = "*CLS";
    static const char* ESE             = "*ESE";
    static const char* ESE_Q           = "*ESE?";
    static const char* ESR_Q           = "*ESR?";
    static const char* IDN_Q           = "*IDN?";
    static const char* OPC             = "*OPC";
    static const char* OPC_Q           = "*OPC?";
    static const char* RCL             = "*RCL";
    static const char* RST             = "*RST";
    static const char* SAV             = "*SAV";
    static const char* SRE             = "*SRE";
    static const char* SRE_Q           = "*SRE?";
    static const char* STB_Q           = "*STB?";
    static const char* TST_Q           = "*TST?";
    static const char* WAI             = "*WAI";
}

// ---- Chapter 2: Common Cassette Commands ----
namespace Cassette {
    static const char* BUSY_Q          = ":BUSY?";
    static const char* CONF_Q          = ":CONF?";
    static const char* DEV_INFO_Q      = ":DEV:INFO?";
    static const char* FAULT_DEV_Q     = ":FAUL:DEV?";
    static const char* FAULT_SLOT_Q    = ":FAUL:SLOT?";
    static const char* INFO_Q          = ":INFO?";
    static const char* LOCK            = ":LOCK";
    static const char* LOCK_Q          = ":LOCK?";
    static const char* RESET           = ":RES";
    static const char* STATUS_DEV_Q    = ":STAT:DEV?";
    static const char* STATUS_RESET    = ":STAT:RES?";
    static const char* SYS_ERR_Q       = ":SYST:ERR?";
    static const char* TEST_Q          = ":TEST?";
}

// ---- Chapter 3: System Commands (port 8100) ----
namespace System {
    static const char* SUPER_EXIT      = ":SUP:EXIT";
    static const char* SUPER_LAUNCH    = ":SUP:LAUN";
    static const char* SUPER_STATUS_Q  = ":SUP:STAT?";
    static const char* SYS_DATE        = ":SYST:DAT";
    static const char* SYS_DATE_Q      = ":SYST:DAT?";
    static const char* SYS_ERR_Q       = ":SYST:ERR?";
    static const char* SYS_FAULT_CHASSIS_Q = ":SYST:FAUL:CHAS?";
    static const char* SYS_FAULT_SUMMARY_Q = ":SYST:FAUL:SUMM?";
    static const char* SYS_GPIB        = ":SYST:GPIB";
    static const char* SYS_GPIB_Q      = ":SYST:GPIB?";
    static const char* SYS_INFO_Q      = ":SYST:INFO?";
    static const char* SYS_INTLOCK     = ":SYST:INTL";
    static const char* SYS_INTLOCK_Q   = ":SYST:INTL?";
    static const char* SYS_INTLOCK_STATE_Q = ":SYST:INTL:STAT?";
    static const char* SYS_INVENTORY_Q = ":SYST:INV?";
    static const char* SYS_IP_LIST_Q   = ":SYST:IP:LIST?";
    static const char* SYS_LAYOUT_Q    = ":SYST:LAY?";
    static const char* SYS_LAYOUT_PORT_Q = ":SYST:LAY:PORT?";
    static const char* SYS_LEGACY_MODE = ":SYST:LEGA:MODE";
    static const char* SYS_LEGACY_MODE_Q = ":SYST:LEGA:MODE?";
    static const char* SYS_LICENSE_Q   = ":SYST:LIC?";
    static const char* SYS_LOCK_RELEASE = ":SYST:LOCK:RELE";
    static const char* SYS_SHUTDOWN    = ":SYST:SHUT";
    static const char* SYS_STATUS_Q    = ":SYST:STAT?";
    static const char* SYS_TEMP_Q      = ":SYST:TEMP?";
    static const char* SYS_TIME_Q      = ":SYST:TIM?";
}

// ---- Chapter 4: System Integration / Factory Commands ----
namespace Factory {
    static const char* CAL_Q           = ":FACT:CAL?";
    static const char* CAL_DATE_Q      = ":FACT:CAL:DAT?";
    static const char* COMMIT          = ":FACT:COMM";
    static const char* CONF_BIDIR      = ":FACT:CONF:BID";
    static const char* CONF_BIDIR_Q    = ":FACT:CONF:BID?";
    static const char* CONF_CORE       = ":FACT:CONF:CORE";
    static const char* CONF_CORE_Q     = ":FACT:CONF:CORE?";
    static const char* CONF_LPOW       = ":FACT:CONF:LPOW";
    static const char* CONF_LPOW_Q     = ":FACT:CONF:LPOW?";
    static const char* CONF_OPM        = ":FACT:CONF:OPM";
    static const char* CONF_OPM_Q      = ":FACT:CONF:OPM?";
    static const char* CONF_SWITCH     = ":FACT:CONF:SWIT";
    static const char* CONF_SWITCH_Q   = ":FACT:CONF:SWIT?";
    static const char* CONF_SWITCH_SIZE = ":FACT:CONF:SWIT:SIZE";
    static const char* MEAS_FP_DIST_Q  = ":FACT:MEAS:FPD?";
    static const char* MEAS_FP_LOSS_Q  = ":FACT:MEAS:FPL?";
    static const char* MEAS_FP_RATIO_Q = ":FACT:MEAS:FPR?";
    static const char* MEAS_LOOP_Q     = ":FACT:MEAS:LOOP?";
    static const char* MEAS_OPMP_Q     = ":FACT:MEAS:OPMP?";
    static const char* MEAS_RANGE_Q    = ":FACT:MEAS:RANG?";
    static const char* MEAS_START      = ":FACT:MEAS:STAR";
    static const char* MEAS_SW_DIST_Q  = ":FACT:MEAS:SWD?";
    static const char* MEAS_SW_LOSS_Q  = ":FACT:MEAS:SWL?";
    static const char* SETUP_SWITCH    = ":FACT:SET:SWIT";
    static const char* SETUP_SWITCH_Q  = ":FACT:SET:SWIT?";
    static const char* RESET           = ":FACT:RES";
}

// ---- Chapter 5: PCT Commands ----
namespace PCT {
    static const char* ACTIVE_Q        = "*ACT?";
    static const char* REM             = "*REM";

    // Measure
    static const char* MEAS_ALL_Q      = ":MEAS:ALL?";
    static const char* MEAS_DIST_Q     = ":MEAS:DIST?";
    static const char* MEAS_FASTIL     = ":MEAS:FAST";
    static const char* MEAS_FASTIL_Q   = ":MEAS:FAST?";
    static const char* MEAS_HELIX      = ":MEAS:HEL";
    static const char* MEAS_HELIX_Q    = ":MEAS:HEL?";
    static const char* MEAS_IL_Q       = ":MEAS:IL?";
    static const char* MEAS_ILLA       = ":MEAS:ILLA";
    static const char* MEAS_ILLA_Q     = ":MEAS:ILLA?";
    static const char* MEAS_LENGTH_Q   = ":MEAS:LENG?";
    static const char* MEAS_ORL_Q      = ":MEAS:ORL?";
    static const char* MEAS_ORL_PRESET_Q = ":MEAS:ORL:PRES?";
    static const char* MEAS_ORL_SETUP  = ":MEAS:ORL:SET";
    static const char* MEAS_ORL_SETUP_Q = ":MEAS:ORL:SET?";
    static const char* MEAS_ORL_ZONE_Q = ":MEAS:ORL:ZON?";
    static const char* MEAS_POWER_Q    = ":MEAS:POW?";
    static const char* MEAS_REF2STEP   = ":MEAS:REF2";
    static const char* MEAS_REF2STEP_Q = ":MEAS:REF2?";
    static const char* MEAS_REFALT     = ":MEAS:REFA";
    static const char* MEAS_REFALT_Q   = ":MEAS:REFA?";
    static const char* MEAS_RESET      = ":MEAS:RES";
    static const char* MEAS_SEIL       = ":MEAS:SEIL";
    static const char* MEAS_SEIL_Q     = ":MEAS:SEIL?";
    static const char* MEAS_START      = ":MEAS:STAR";
    static const char* MEAS_STATE_Q    = ":MEAS:STAT?";
    static const char* MEAS_STOP       = ":MEAS:STOP";

    // Path
    static const char* PATH_BIDIR      = ":PATH:BIDIR";
    static const char* PATH_BIDIR_Q    = ":PATH:BIDIR?";
    static const char* PATH_CHANNEL    = ":PATH:CHAN";
    static const char* PATH_CHANNEL_Q  = ":PATH:CHAN?";
    static const char* PATH_CHANNEL_AVA_Q = ":PATH:CHAN:AVA?";
    static const char* PATH_CONNECTION = ":PATH:CONN";
    static const char* PATH_CONNECTION_Q = ":PATH:CONN?";
    static const char* PATH_DUT_LENGTH = ":PATH:DUT:LENG";
    static const char* PATH_DUT_LENGTH_Q = ":PATH:DUT:LENG?";
    static const char* PATH_DUT_LENGTH_AUTO = ":PATH:DUT:LENG:AUTO";
    static const char* PATH_DUT_LENGTH_AUTO_Q = ":PATH:DUT:LENG:AUTO?";
    static const char* PATH_EOF_MIN    = ":PATH:EOF:MIN";
    static const char* PATH_EOF_MIN_Q  = ":PATH:EOF:MIN?";
    static const char* PATH_JUMPER_IL  = ":PATH:JUMP:IL";
    static const char* PATH_JUMPER_IL_Q = ":PATH:JUMP:IL?";
    static const char* PATH_JUMPER_IL_AUTO = ":PATH:JUMP:IL:AUTO";
    static const char* PATH_JUMPER_IL_AUTO_Q = ":PATH:JUMP:IL:AUTO?";
    static const char* PATH_JUMPER_LENGTH = ":PATH:JUMP:LENG";
    static const char* PATH_JUMPER_LENGTH_Q = ":PATH:JUMP:LENG?";
    static const char* PATH_JUMPER_LENGTH_AUTO = ":PATH:JUMP:LENG:AUTO";
    static const char* PATH_JUMPER_LENGTH_AUTO_Q = ":PATH:JUMP:LENG:AUTO?";
    static const char* PATH_JUMPER_RESET = ":PATH:JUMP:RES";
    static const char* PATH_JUMPER_RESET_MEAS = ":PATH:JUMP:RES:MEAS";
    static const char* PATH_LAUNCH     = ":PATH:LAUN";
    static const char* PATH_LAUNCH_Q   = ":PATH:LAUN?";
    static const char* PATH_LAUNCH_AVA_Q = ":PATH:LAUN:AVA?";
    static const char* PATH_LIST       = ":PATH:LIST";
    static const char* PATH_LIST_Q     = ":PATH:LIST?";
    static const char* PATH_RECEIVE    = ":PATH:REC";
    static const char* PATH_RECEIVE_Q  = ":PATH:REC?";

    // Port Map
    static const char* PMAP_ENABLE     = ":PMAP:ENAB";
    static const char* PMAP_ENABLE_Q   = ":PMAP:ENAB?";
    static const char* PMAP_MEAS_ALL   = ":PMAP:MEAS:ALL";
    static const char* PMAP_MEAS_LIVE  = ":PMAP:MEAS:LIVE";
    static const char* PMAP_MEAS_LIVE_Q = ":PMAP:MEAS:LIVE?";
    static const char* PMAP_MEAS_VALID = ":PMAP:MEAS:VAL";
    static const char* PMAP_MEAS_VALID_Q = ":PMAP:MEAS:VAL?";
    static const char* PMAP_PATH_LINK_Q = ":PMAP:PATH:LINK?";
    static const char* PMAP_PATH_SELECT = ":PMAP:PATH:SEL";
    static const char* PMAP_PATH_SELECT_Q = ":PMAP:PATH:SEL?";
    static const char* PMAP_PATH_SIZE_Q = ":PMAP:PATH:SIZE?";
    static const char* PMAP_SETUP_FIRST_Q = ":PMAP:SET:FIRS?";
    static const char* PMAP_SETUP_INIT_LIST = ":PMAP:SET:INIT:LIST";
    static const char* PMAP_SETUP_INIT_RANGE = ":PMAP:SET:INIT:RANG";
    static const char* PMAP_SETUP_LAST_Q = ":PMAP:SET:LAST?";
    static const char* PMAP_SETUP_LINK = ":PMAP:SET:LINK";
    static const char* PMAP_SETUP_LIST_Q = ":PMAP:SET:LIST?";
    static const char* PMAP_SETUP_LOCK = ":PMAP:SET:LOCK";
    static const char* PMAP_SETUP_LOCK_Q = ":PMAP:SET:LOCK?";
    static const char* PMAP_SETUP_MODE_Q = ":PMAP:SET:MODE?";
    static const char* PMAP_SETUP_RESET = ":PMAP:SET:RES";
    static const char* PMAP_SETUP_SIZE_Q = ":PMAP:SET:SIZE?";

    // Sense
    static const char* SENS_ATIME      = ":SENS:ATIM";
    static const char* SENS_ATIME_Q    = ":SENS:ATIM?";
    static const char* SENS_ATIME_AVA_Q = ":SENS:ATIM:AVA?";
    static const char* SENS_FUNC       = ":SENS:FUNC";
    static const char* SENS_FUNC_Q     = ":SENS:FUNC?";
    static const char* SENS_ILONLY     = ":SENS:ILON";
    static const char* SENS_ILONLY_Q   = ":SENS:ILON?";
    static const char* SENS_OPM       = ":SENS:OPM";
    static const char* SENS_OPM_Q     = ":SENS:OPM?";
    static const char* SENS_RANGE     = ":SENS:RANG";
    static const char* SENS_RANGE_Q   = ":SENS:RANG?";
    static const char* SENS_TEMP      = ":SENS:TEMP";
    static const char* SENS_TEMP_Q    = ":SENS:TEMP?";

    // Source
    static const char* SOUR_CONTINUOUS = ":SOUR:CONT";
    static const char* SOUR_CONTINUOUS_Q = ":SOUR:CONT?";
    static const char* SOUR_LIST      = ":SOUR:LIST";
    static const char* SOUR_LIST_Q    = ":SOUR:LIST?";
    static const char* SOUR_WARMUP    = ":SOUR:WARM";
    static const char* SOUR_WARMUP_Q  = ":SOUR:WARM?";
    static const char* SOUR_WAV       = ":SOUR:WAV";
    static const char* SOUR_WAV_Q     = ":SOUR:WAV?";
    static const char* SOUR_WAV_AVA_Q = ":SOUR:WAV:AVA?";

    // System Warning
    static const char* SYS_WARNING_Q  = ":SYST:WARN?";

    // OPM Commands
    static const char* FETCH_LOSS_Q   = ":FETC:LOSS?";
    static const char* FETCH_ORL_Q    = ":FETC:ORL?";
    static const char* FETCH_POWER_Q  = ":FETC:POW?";
    static const char* SENS_POW_DARK  = ":SENS:POW:DARK";
    static const char* SENS_POW_DARK_Q = ":SENS:POW:DARK?";
    static const char* SENS_POW_DARK_FACTORY = ":SENS:POW:DARK:FACT";
    static const char* SENS_POW_MODE  = ":SENS:POW:MODE";
    static const char* SENS_POW_MODE_Q = ":SENS:POW:MODE?";
}

} // namespace SCPI
} // namespace ViaviPCT4All
