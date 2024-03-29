#pragma once
typedef union
{
    struct
    {
        uint16_t year;
        uint8_t day_of_month;
        uint8_t month;
        uint8_t hour;
        uint8_t week_day;
        uint8_t second;
        uint8_t minute;
        int8_t deviation_lsb;
        uint8_t hundreds_of_second;
        uint8_t clock_status;
        int8_t deviation_msb;
    };
    std::array<std::uint16_t, 6> buffer;
} __attribute__((packed)) han_clock_t;
enum HAN_REGISTERS
{
    CLOCK = 0x0001,
    DEVICE_SERIAL_NUMBER = 0x0002,
    MANUFACTURER_MODEL_CODES_AND_YEAR = 0x0003,
    ACTIVE_CORE_FIRMWARE_ID = 0x0004,
    ACTIVE_APP_FIRMWARE_ID = 0x0005,
    ACTIVE_COM_FIRMWARE_ID = 0x0006,
    HAN_INTERFACE_MODBUS_ADDRESS = 0x0007,
    HAN_INTERFACE_ACCESS_PROFILE = 0x0008,
    STATUS_CONTROL_1 = 0x0009,
    ACTIVITY_CALENDAR_ACTIVE_NAME = 0x000A,
    CURRENTLY_ACTIVE_TARIFF = 0x000B,
    ACTIVE_DEMAND_CONTROL_THRESHOLD_T1 = 0x000C,
    ACTIVE_DEMAND_CONTROL_THRESHOLD_T2 = 0x000D,
    ACTIVE_DEMAND_CONTROL_THRESHOLD_T3 = 0x000E,
    ACTIVE_DEMAND_CONTROL_THRESHOLD_T4 = 0x000F,
    ACTIVE_DEMAND_CONTROL_THRESHOLD_T5 = 0x0010,
    ACTIVE_DEMAND_CONTROL_THRESHOLD_T6 = 0x0011,
    CURRENTLY_APPARENT_POWER_THRESHOLD = 0x0012,
    DEMAND_MANAGEMENT_STATUS = 0x0013,
    DEMAND_MANAGEMENT_PERIOD_DEFINITION = 0x0014,
    RESIDUAL_POWER_THRESHOLD = 0x0015,
    ACTIVE_ENERGY_IMPORT_PLUS_A = 0x0016,
    ACTIVE_ENERGY_EXPORT_MINUS_A = 0x0017,
    REACTIVE_ENERGY_QI_PLUS_RI = 0x0018,
    REACTIVE_ENERGY_QII_PLUS_RC = 0x0019,
    REACTIVE_ENERGY_QIII_MINUS_RI = 0x001A,
    REACTIVE_ENERGY_QIV_MINUS_RC = 0x001B,
    ACTIVE_ENERGY_IMPORT_PLUS_A_L1 = 0x001C,
    ACTIVE_ENERGY_IMPORT_PLUS_A_L2 = 0x001D,
    ACTIVE_ENERGY_IMPORT_PLUS_A_L3 = 0x001E,
    ACTIVE_ENERGY_EXPORT_MINUS_A_L1 = 0x001F,
    ACTIVE_ENERGY_EXPORT_MINUS_A_L21 = 0x0020,
    ACTIVE_ENERGY_EXPORT_MINUS_A_L31 = 0x0021,
    MAX_DEMAND_ACTIVE_POWER_PLUS_QIR_PLUS_PLUS_QIV = 0x0022,
    MAX_DEMAND_ACTIVE_POWER_PLUS_QIR_PLUS_PLUS_QIV_CAPTURE_TIME = 0x0023,
    MAX_DEMAND_ACTIVE_POWER_QIR_PLUS_I_PLUS_QIII = 0x0024,
    MAX_DEMAND_ACTIVE_POWER_IR_PLUS_I_PLUS_QIII = 0x0025,
    RATE_1_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x0026,
    RATE_2_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x0027,
    RATE_3_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x0028,
    RATE_4_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x0029,
    RATE_5_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x002A,
    RATE_6_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x002B,
    TOTAL_RATE_CONTRACT_1_ACTIVE_ENERGY_PLUS_A = 0x002C,
    RATE_1_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x002D,
    RATE_2_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x002E,
    RATE_3_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x002F,
    RATE_4_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x0030,
    RATE_5_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x0031,
    RATE_6_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x0032,
    TOTAL_RATE_CONTRACT_1_ACTIVE_ENERGY_MINUS_A = 0x0033,
    RATE_1_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x0034,
    RATE_2_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x0035,
    RATE_3_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x0036,
    RATE_4_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x0037,
    RATE_5_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x0038,
    RATE_6_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x0039,
    TOTAL_RATE_CONTRACT_1_REACTIVE_ENERGY_QI_PLUS_RI = 0x003A,
    RATE_1_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x003B,
    RATE_2_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x003C,
    RATE_3_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x003D,
    RATE_4_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x003E,
    RATE_5_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x003F,
    RATE_6_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x0040,
    TOTAL_RATE_CONTRACT_1_REACTIVE_ENERGY_QII_PLUS_RC = 0x0041,
    RATE_1_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0042,
    RATE_2_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0043,
    RATE_3_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0044,
    RATE_4_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0045,
    RATE_5_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0046,
    RATE_6_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0047,
    TOTAL_RATE_CONTRACT_1_REACTIVE_ENERGY_QIII_MINUS_RI = 0x0048,
    RATE_1_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x0049,
    RATE_2_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x004A,
    RATE_3_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x004B,
    RATE_4_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x004C,
    RATE_5_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x004D,
    RATE_6_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x004E,
    TOTAL_RATE_CONTRACT_1_REACTIVE_ENERGY_QIV_MINUS_RC = 0x004F,
    RATE_1_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x0050,
    RATE_1_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x0051,
    RATE_2_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x0052,
    RATE_2_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x0053,
    RATE_3_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x0054,
    RATE_3_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x0055,
    RATE_4_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x0056,
    RATE_4_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x0057,
    RATE_5_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x0058,
    RATE_5_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x0059,
    RATE_6_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x005A,
    RATE_6_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x005B,
    TOTAL_RATE_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_LAST_AVERAGE = 0x005C,
    TOTAL_RATE_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_PLUS_CAPTURE_TIME = 0x005D,
    RATE_1_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x005E,
    RATE_1_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x005F,
    RATE_2_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x0060,
    RATE_2_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x0061,
    RATE_3_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x0062,
    RATE_3_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x0063,
    RATE_4_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x0064,
    RATE_4_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x0065,
    RATE_5_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x0066,
    RATE_5_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x0067,
    RATE_6_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x0068,
    RATE_6_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x0069,
    TOTAL_RATE_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_LAST_AVERAGE = 0x006A,
    TOTAL_RATE_CONTRACT_1_MAXIMUM_DEMAND_ACTIVE_POWER_CAPTURE_TIME = 0x006B,
    INSTANTANEOUS_VOLTAGE_L1 = 0x006C,
    INSTANTANEOUS_CURRENT_L1 = 0x006D,
    INSTANTANEOUS_VOLTAGE_L2 = 0x006E,
    INSTANTANEOUS_CURRENT_L2 = 0x006F,
    INSTANTANEOUS_VOLTAGE_L3 = 0x0070,
    INSTANTANEOUS_CURRENT_L3 = 0x0071,
    INSTANTANEOUS_CURRENT_SUM_OF_ALL_PHASES = 0x0072,
    INSTANTANEOUS_ACTIVE_POWER_PLUS_L1 = 0x0073,
    INSTANTANEOUS_ACTIVE_POWER_L1 = 0x0074,
    INSTANTANEOUS_ACTIVE_POWER_PLUS_L2 = 0x0075,
    INSTANTANEOUS_ACTIVE_POWER_L2 = 0x0076,
    INSTANTANEOUS_ACTIVE_POWER_PLUS_L3 = 0x0077,
    INSTANTANEOUS_ACTIVE_POWER_L3 = 0x0078,
    INSTANTANEOUS_ACTIVE_POWER_PLUS_SUM_OF_ALL_PHASES = 0x0079,
    INSTANTANEOUS_ACTIVE_POWER = 0x007A,
    INSTANTANEOUS_POWER_FACTOR = 0x007B,
    INSTANTANEOUS_POWER_FACTOR_L1 = 0x007C,
    INSTANTANEOUS_POWER_FACTOR_L2 = 0x007D,
    INSTANTANEOUS_POWER_FACTOR_L3 = 0x007E,
    INSTANTANEOUS_FREQUENCY = 0x007F,
    LOAD_PROFILE_CONFIGURED_MEASUREMENTS = 0x0080,
    LOAD_PROFILE_CAPTURE_PERIOD = 0x0081,
    LOAD_PROFILE_ENTRIES_IN_USE = 0x0082,
    LOAD_PROFILE_PROFILE_ENTRIES = 0x0083,
    DISCONNECT_CONTROL_STATE = 0x0084,
    DISCONNECTOR_Q_PARAMETER = 0x0085,
    DISCONNECTOR_K_PARAMETER = 0x0086,
};