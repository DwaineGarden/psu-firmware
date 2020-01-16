/*
 * EEZ PSU Firmware
 * Copyright (C) 2015-present, Envox d.o.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once

#include "persist_conf.h"
#include "ioexp.h"
#include "adc.h"
#include "dac.h"
#include "temp_sensor.h"

#define IS_OVP_VALUE(channel, cpv) (&cpv == &channel->ovp)
#define IS_OCP_VALUE(channel, cpv) (&cpv == &channel->ocp)
#define IS_OPP_VALUE(channel, cpv) (&cpv == &channel->opp)

namespace eez {
namespace psu {

namespace calibration {
struct Value;
}

enum DisplayValue {
    DISPLAY_VALUE_VOLTAGE,
    DISPLAY_VALUE_CURRENT,
    DISPLAY_VALUE_POWER
};

enum ChannelFeatures {
    CH_FEATURE_VOLT = (1 << 1),
    CH_FEATURE_CURRENT = (1 << 2),
    CH_FEATURE_POWER = (1 << 3),
    CH_FEATURE_OE = (1 << 4),
    CH_FEATURE_DPROG = (1 << 5),
    CH_FEATURE_LRIPPLE = (1 << 6),
    CH_FEATURE_RPROG = (1 << 7),
    CH_FEATURE_RPOL = (1 << 8)
};

enum TriggerMode {
    TRIGGER_MODE_FIXED,
    TRIGGER_MODE_LIST,
    TRIGGER_MODE_STEP
};

enum TriggerOnListStop {
    TRIGGER_ON_LIST_STOP_OUTPUT_OFF,
    TRIGGER_ON_LIST_STOP_SET_TO_FIRST_STEP,
    TRIGGER_ON_LIST_STOP_SET_TO_LAST_STEP,
    TRIGGER_ON_LIST_STOP_STANDBY
};


enum CurrentRangeSelectionMode {
    CURRENT_RANGE_SELECTION_USE_BOTH,
    CURRENT_RANGE_SELECTION_ALWAYS_HIGH,
    CURRENT_RANGE_SELECTION_ALWAYS_LOW
};

enum CurrentRange {
    CURRENT_RANGE_HIGH,
    CURRENT_RANGE_LOW
};

/// PSU channel.
class Channel {
    friend class DigitalAnalogConverter;
    friend struct calibration::Value;

public:
    /// Binary flags for the channel calibration configuration.
    struct CalibrationConfigurationFlags {
        /// Is voltage calibrated?
        unsigned u_cal_params_exists : 1; 
        /// Is current in range 0 (5A) calibrated?
        unsigned i_cal_params_exists_range_high : 1;
        /// Is current in range 1 (500mA) calibrated?
        unsigned i_cal_params_exists_range_low : 1;
    };

    /// Calibration parameters for the single point.
    struct CalibrationValuePointConfiguration {
        /// Value set on DAC by the calibration module.
        float dac;
        /// Real value, in volts, set by the user who reads it on the instrument (voltmeter and ampermeter).
        float val;
        /// Value read from ADC.
        float adc;
    };

    /// Calibration parameters for the voltage and current.
    /// There are three points defined: `min`, `mid` and `max`.
    /// Only `min` and `max` are used in actual calculations -
    /// `mid` is only used for the validity checking.
    /// Here is how `DAC` value is calculated from the `real_value` set by user:
    /// `DAC = min.dac + (real_value - min.val) * (max.dac - min.dac) / (max.val - min.val);`
    /// And here is how `real_value` is calculated from the `ADC` value:
    /// `real_value = min.val + (ADC - min.adc) * (max.val - min.val) / (max.adc - min.adc);`
    struct CalibrationValueConfiguration {
        /// Min point.
        CalibrationValuePointConfiguration min;
        /// Mid point.
        CalibrationValuePointConfiguration mid;
        /// Max point.
        CalibrationValuePointConfiguration max;

        /// Real min after calibration
        float minPossible;

        /// Real max after calibration
        float maxPossible;
    };

    /// A structure where calibration parameters for the channel are stored.
    struct CalibrationConfiguration {
        /// Used by the persist_conf.
        persist_conf::BlockHeader header;

        /// Flags
        CalibrationConfigurationFlags flags;

        /// Calibration parameters for the voltage.
        CalibrationValueConfiguration u; 

        /// Calibration parameters for the currents in both ranges.
        CalibrationValueConfiguration i[2];

        /// Date when calibration is saved.
        /// Automatically set if RTC is present.
        /// Format is YYYYMMDD. 
        char calibration_date[8 + 1];

        /// Remark about calibration set by user.
        char calibration_remark[CALIBRATION_REMARK_MAX_LENGTH + 1];
    };

    /// Binary flags for the channel protection configuration
    struct ProtectionConfigurationFlags {
        /// Is OVP enabled?
        unsigned u_state : 1;
        /// Is OCP enabled?
        unsigned i_state : 1;
        /// Is OPP enabled?
        unsigned p_state : 1;
    };

    /// Channel OVP, OVP and OPP configuration parameters like level and delay.
    struct ChannelProtectionConfiguration {
        persist_conf::BlockHeader header;

        ProtectionConfigurationFlags flags;

        float u_delay;
        float u_level;
        float i_delay;
        float p_delay;
        float p_level;
    };

    /// Channel binary flags like output enabled, sense enabled, ...
    struct Flags {
        unsigned outputEnabled : 1;
        unsigned afterBootOutputEnabled: 1;
        unsigned dpOn : 1;
        unsigned senseEnabled : 1;
        unsigned cvMode : 1;
        unsigned ccMode : 1;
        unsigned powerOk : 1;
        unsigned _calEnabled : 1;
        unsigned rprogEnabled: 1;
        unsigned lrippleEnabled: 1;
        unsigned lrippleAutoEnabled: 1;
        unsigned rpol : 1; // remote sense reverse polarity is detected
        unsigned displayValue1: 2;
        unsigned displayValue2: 2;
        unsigned voltageTriggerMode: 2;
        unsigned currentTriggerMode: 2;
        unsigned triggerOutputState: 1;
        unsigned triggerOnListStop: 3;
        unsigned currentRangeSelectionMode: 2; // see enum CurrentRangeSelectionMode
        unsigned autoSelectCurrentRange: 1; // switch between 5A/0.5A depending on Imon
        unsigned currentCurrentRange: 1; // 0: 5A, 1:0.5A
    };

    /// Voltage and current data set and measured during runtime.
    struct Value {
        float set;

        int16_t mon_adc;

        bool mon_measured;

        float mon;
        float mon_last;
        int8_t mon_index;
        float mon_arr[NUM_ADC_AVERAGING_VALUES];
        float mon_total;

        float mon_dac;
        int8_t mon_dac_index;
        float mon_dac_arr[NUM_ADC_AVERAGING_VALUES];
        float mon_dac_total;

        float step;
        float limit;

        float min;
        float def;
        float max;

        void init(float set_, float step_, float limit_);
        void resetMonValues();
        void addMonDacValue(float value);
        void addMonValue(float value);
    };

    /// Runtime protection binary flags (alarmed, tripped)
    struct ProtectionFlags {
        unsigned alarmed : 1;
        unsigned tripped : 1;
    };

    /// Runtime protection values    
    struct ProtectionValue {
        ProtectionFlags flags;
        uint32_t alarm_started;
    };

#ifdef EEZ_PSU_SIMULATOR
    /// Per channel simulator data
    struct Simulator {
        bool oe;
        bool load_enabled;
        float load;
        float u_set;
        float u_dac;
        float i_set;
        float i_dac;
        float temperature[temp_sensor::NUM_TEMP_SENSORS];
        float voltProgExt;

        void setLoadEnabled(bool value);
        bool getLoadEnabled();

        void setLoad(float value);
        float getLoad();

        void setVoltProgExt(float value);
        float getVoltProgExt();
    };
#endif // EEZ_PSU_SIMULATOR

    /// Get channel instance
    /// \param channel_index Zero based channel index, greater then or equal to 0 and less then CH_MAX.
    /// \returns Reference to channel.
    static Channel &get(int channel_index);

    /// Save and disable OE for all the channels.
    static void saveAndDisableOE();

    /// Restore previously saved OE state for all the channels.
    static void restoreOE();

    ///
    static char *getChannelsInfo(char *p);
    static char *getChannelsInfoShort(char *p);

    /// Channel index. Starts from 1.
    uint8_t index;

    uint8_t boardRevision;

    uint8_t ioexp_iodir;
    uint8_t ioexp_gpio_init;
    uint8_t isolator_pin;
    uint8_t ioexp_pin;
    uint8_t convend_pin;
    uint8_t adc_pin;
    uint8_t dac_pin;
#if EEZ_PSU_SELECTED_REVISION == EEZ_PSU_REVISION_R1B9
    uint8_t bp_led_out_plus;
    uint8_t bp_led_out_minus;
    uint8_t bp_led_sense_plus;
    uint8_t bp_led_sense_minus;
#elif EEZ_PSU_SELECTED_REVISION == EEZ_PSU_REVISION_R3B4 || EEZ_PSU_SELECTED_REVISION == EEZ_PSU_REVISION_R5B12
    uint8_t bp_led_out;
    uint8_t bp_led_sense;
    uint8_t bp_led_prog;
#endif
    uint8_t bp_relay_sense;
    uint8_t cc_led_pin;
    uint8_t cv_led_pin;

    float U_MIN_STEP;
    float U_DEF_STEP;
    float U_MAX_STEP;
    float U_CAL_VAL_MIN;
    float U_CAL_VAL_MID;
    float U_CAL_VAL_MAX;
    float U_CURR_CAL; // voltage level during current calibration

    bool OVP_DEFAULT_STATE;
    float OVP_MIN_DELAY;
    float OVP_DEFAULT_DELAY;
    float OVP_MAX_DELAY;

    float I_MIN_STEP;
    float I_DEF_STEP;
    float I_MAX_STEP;
    float I_CAL_VAL_MIN;
    float I_CAL_VAL_MID;
    float I_CAL_VAL_MAX;
    float I_VOLT_CAL; // current level during voltage calibration

    bool OCP_DEFAULT_STATE;
    float OCP_MIN_DELAY;
    float OCP_DEFAULT_DELAY;
    float OCP_MAX_DELAY;

    bool OPP_DEFAULT_STATE;
    float OPP_MIN_DELAY;
    float OPP_DEFAULT_DELAY;
    float OPP_MAX_DELAY;
    float OPP_MIN_LEVEL;
    float OPP_DEFAULT_LEVEL;
    float OPP_MAX_LEVEL;

    float SOA_VIN;
    float SOA_PREG_CURR;
    float SOA_POSTREG_PTOT;

    float PTOT;

    IOExpander ioexp;
    AnalogDigitalConverter adc;
    DigitalAnalogConverter dac;

    Flags flags;

    Value u;
    Value i;

    float p_limit;

    CalibrationConfiguration cal_conf;
    ChannelProtectionConfiguration prot_conf;

    ProtectionValue ovp;
    ProtectionValue ocp;
    ProtectionValue opp;

    ontime::Counter onTimeCounter;

    float ytViewRate;

#ifdef EEZ_PSU_SIMULATOR
    Simulator simulator;
#endif // EEZ_PSU_SIMULATOR

    Channel(
        uint8_t index,
        uint8_t boardRevision, uint8_t ioexp_iodir, uint8_t ioexp_gpio_init, uint8_t IO_BIT_OUT_SET_100_PERCENT_, uint8_t IO_BIT_OUT_EXT_PROG_, float VOLTAGE_GND_OFFSET_, float CURRENT_GND_OFFSET_,
        uint8_t isolator_pin, uint8_t ioexp_pin, uint8_t convend_pin, uint8_t adc_pin, uint8_t dac_pin,
#if EEZ_PSU_SELECTED_REVISION == EEZ_PSU_REVISION_R1B9
        uint8_t bp_led_out_plus, uint8_t bp_led_out_minus, uint8_t bp_led_sense_plus, uint8_t bp_led_sense_minus, uint8_t bp_relay_sense,
#elif EEZ_PSU_SELECTED_REVISION == EEZ_PSU_REVISION_R3B4 || EEZ_PSU_SELECTED_REVISION == EEZ_PSU_REVISION_R5B12
        uint8_t bp_led_out, uint8_t bp_led_sense, uint8_t bp_relay_sense, uint8_t bp_led_prog,
#endif
        uint8_t cc_led_pin, uint8_t cv_led_pin,
        float U_MIN, float U_DEF, float U_MAX, float U_MAX_CONF, float U_MIN_STEP, float U_DEF_STEP, float U_MAX_STEP, float U_CAL_VAL_MIN, float U_CAL_VAL_MID, float U_CAL_VAL_MAX, float U_CURR_CAL,
        bool OVP_DEFAULT_STATE, float OVP_MIN_DELAY, float OVP_DEFAULT_DELAY, float OVP_MAX_DELAY,
        float I_MIN, float I_DEF, float I_MAX, float I_MAX_CONF, float I_MIN_STEP, float I_DEF_STEP, float I_MAX_STEP, float I_CAL_VAL_MIN, float I_CAL_VAL_MID, float I_CAL_VAL_MAX, float I_VOLT_CAL,
        bool OCP_DEFAULT_STATE, float OCP_MIN_DELAY, float OCP_DEFAULT_DELAY, float OCP_MAX_DELAY,
        bool OPP_DEFAULT_STATE, float OPP_MIN_DELAY, float OPP_DEFAULT_DELAY, float OPP_MAX_DELAY, float OPP_MIN_LEVEL, float OPP_DEFAULT_LEVEL, float OPP_MAX_LEVEL,
        float SOA_VIN, float SOA_PREG_CURR, float SOA_POSTREG_PTOT, float PTOT);

    /// Initialize channel and underlying hardware.
    /// Makes a required tests, for example ADC, DAC and IO Expander tests.
    void init();

    /// Reset the channel to default values.
    void reset();

    /// Clear channel calibration configuration.
    void clearCalibrationConf();

    /// Test the channel.
    bool test();

    /// Is channel power ok (state of PWRGOOD bit in IO Expander)?
    bool isPowerOk();
    
    /// Is channel test failed?
    bool isTestFailed();
    
    /// Is channel test ok?
    bool isTestOk();

    /// Is channel ready to work with?
    bool isOk();

    /// Called by main loop, used for channel maintenance.
    void tick(uint32_t tick_usec);

    /// Called from IO expander interrupt routine.
    /// @param gpio State of IO expander GPIO register.
    /// @param adc_data ADC snapshot data.
    void eventAdcData(int16_t adc_data, bool startAgain = true);
    void eventGpio(uint8_t gpio);

    /// Called when device power is turned off, so channel
    /// can do its own housekeeping.
    void onPowerDown();

    /// Force ADC read of u.mon_dac and i.mon_dac.
    void adcReadMonDac();

    /// Force ADC read of all values: u.mon, u.mon_dac, i.mon and i.mon_dac.
    void adcReadAll();

    /// Force update of all channel state (u.set, i.set, output enable, remote sensing, ...).
    /// This is called when channel is recovering from hardware failure.
    void update();

    /// Enable/disable channel output.
    void outputEnable(bool enable);

    void afterBootOutputEnable();

    /// Is channel output enabled?
    bool isOutputEnabled();

    /// Enable/disable channel output depending on inhibited state
    void onInhibitedChanged(bool inhibited);

    /// Enable/disable channel calibration.
    void calibrationEnable(bool enabled);
    void calibrationEnableNoEvent(bool enabled);

    /// Is channel calibration enabled?
    bool isCalibrationEnabled();

    /// Enable/disable remote sensing.
    void remoteSensingEnable(bool enable);

    /// Is remote sensing enabled?
    bool isRemoteSensingEnabled();

    /// Enable/disable remote programming.
    void remoteProgrammingEnable(bool enable);

    /// Is remote programming enabled?
    bool isRemoteProgrammingEnabled();

    /// Enable/disable low ripple mode.
    bool lowRippleEnable(bool enable);

    /// Enable/disable low ripple auto mode.
    void lowRippleAutoEnable(bool enable);

    /// Is low ripple mode allowed?
    bool isLowRippleAllowed(uint32_t tick_usec);

    /// Is low ripple mode enabled?
    bool isLowRippleEnabled();

    /// Is low ripple auto mode enabled?
    bool isLowRippleAutoEnabled();

    /// Set channel voltage level.
    void setVoltage(float voltage);

    /// Set channel current level
    void setCurrent(float current);

    /// Is channel calibrated, both voltage and current?
    bool isCalibrationExists();

    /// Is OVP, OCP or OPP tripped?
    bool isTripped();

    /// Clear channel protection tripp state.
    void clearProtection(bool clearOTP = true);

    /// Disable protection for this channel
    void disableProtection();

    /// Turn on/off bit in SCPI Questinable Instrument Isummary register for this channel.
    void setQuesBits(int bit_mask, bool on);

    /// Turn on/off bit in SCPI Operational Instrument Isummary register for this channel.
    void setOperBits(int bit_mask, bool on);

    /// Is channel in CV (constant voltage) mode?
    bool isCvMode() { return flags.cvMode && !flags.ccMode; }

    /// Is channel in CC (constant current) mode?
    bool isCcMode() { return flags.ccMode && !flags.cvMode; }

    /// Returns "CC", "CV" or "UR"
    char *getCvModeStr();

    /// Remap ADC data value to actual voltage value
    float remapAdcDataToVoltage(int16_t adc_data);

    /// Remap ADC data value to actual current value (use calibration if configured).
    float remapAdcDataToCurrent(int16_t adc_data);

    /// Remap voltage value to ADC data value (use calibration if configured).
    int16_t remapVoltageToAdcData(float value);

    /// Remap current value to ADC data value (use calibration if configured).
    int16_t remapCurrentToAdcData(float value);

    /// Returns name of the board revison of this channel.
    const char *getBoardRevisionName();

    /// Returns features present (check ChannelFeatures) in board revision of this channel.
    uint16_t getFeatures();

    /// Returns currently set voltage limit
    float getVoltageLimit() const;

    /// Returns max. voltage limit
    float getVoltageMaxLimit() const;

    /// Change voltage limit, it will adjust U_SET if necessary.
    void setVoltageLimit(float limit);

    /// Returns currently set current limit
    float getCurrentLimit() const;

    /// Change current limit, it will adjust I_SET if necessary.
    void setCurrentLimit(float limit);

    /// Returns ERR_MAX_CURRENT if max. current is limited or i.max if not
    float getMaxCurrentLimit() const;

    /// Returns true if max current is limited to ERR_MAX_CURRENT.
    bool isMaxCurrentLimited() const;

    /// Returns max. current limit cause
    MaxCurrentLimitCause getMaxCurrentLimitCause() const;

    /// Set current max. limit to ERR_MAX_CURRENT
    void limitMaxCurrent(MaxCurrentLimitCause cause);
    
    /// Unset current max. limit 
    void unlimitMaxCurrent();

    /// Returns currently set power limit
    float getPowerLimit() const;

    /// Returns max. power limit
    float getPowerMaxLimit() const;

    /// Change power limit, it will adjust U_SET or I_SET if necessary.
    void setPowerLimit(float limit);

    bool isVoltageBalanced() { return !util::isNaN(uBeforeBalancing); }
    bool isCurrentBalanced() { return !util::isNaN(iBeforeBalancing); }
    float getUSetUnbalanced() { return isVoltageBalanced() ? uBeforeBalancing : u.set; }
    float getISetUnbalanced() { return isCurrentBalanced() ? iBeforeBalancing : i.set; }

    int getCurrentHistoryValuePosition() { return historyPosition; }
    float getUMonHistory(int position) const { return uHistory[position]; }
    float getIMonHistory(int position) const { return iHistory[position]; }

    void resetHistory();

    TriggerMode getVoltageTriggerMode();
    void setVoltageTriggerMode(TriggerMode mode);

    TriggerMode getCurrentTriggerMode();
    void setCurrentTriggerMode(TriggerMode mode);

    bool getTriggerOutputState();
    void setTriggerOutputState(bool enabled);

    TriggerOnListStop getTriggerOnListStop();
    void setTriggerOnListStop(TriggerOnListStop value);

    bool hasSupportForCurrentDualRange() const { return boardRevision == CH_BOARD_REVISION_R5B12; }
    void setCurrentRangeSelectionMode(CurrentRangeSelectionMode mode);
    CurrentRangeSelectionMode getCurrentRangeSelectionMode() { return (CurrentRangeSelectionMode)flags.currentRangeSelectionMode; }
    void enableAutoSelectCurrentRange(bool enable);
    bool isAutoSelectCurrentRangeEnabled() { return flags.autoSelectCurrentRange ? true : false; }
    bool isCurrentLowRangeAllowed();
    float getDualRangeMax();
    void setCurrentRange(uint8_t currentRange);

private:
    bool delayed_dp_off;
    uint32_t delayed_dp_off_start;
    bool delayLowRippleCheck;
    uint32_t outputEnableStartTime;
    uint32_t dpNegMonitoringTime;

    float U_MIN;
    float U_DEF;
    float U_MAX;
    float U_MAX_CONF;

    float I_MIN;
    float I_DEF;
    float I_MAX;
    float I_MAX_CONF;

    float uBeforeBalancing;
    float iBeforeBalancing;

    MaxCurrentLimitCause maxCurrentLimitCause;

    //int negligibleAdcDiffForVoltage2;
    //int negligibleAdcDiffForVoltage3;
    //int negligibleAdcDiffForCurrent;

    float uHistory[CHANNEL_HISTORY_SIZE];
    float iHistory[CHANNEL_HISTORY_SIZE];
    int historyPosition;
    uint32_t historyLastTick;

    float VOLTAGE_GND_OFFSET;
    float CURRENT_GND_OFFSET;

    void clearProtectionConf();
    void protectionEnter(ProtectionValue &cpv);
    void protectionCheck(ProtectionValue &cpv);
    void protectionCheck();

    void doCalibrationEnable(bool enable);
    void calibrationFindVoltageRange(float minDac, float minVal, float minAdc, float maxDac, float maxVal, float maxAdc, float *min, float *max);
    void calibrationFindCurrentRange(float minDac, float minVal, float minAdc, float maxDac, float maxVal, float maxAdc, float *min, float *max);
    bool isVoltageCalibrationEnabled();
    bool isCurrentCalibrationEnabled();

    void adcDataIsReady(int16_t data, bool startAgain);
    
    void voltageBalancing();
    void currentBalancing();

    void restoreVoltageToValueBeforeBalancing();
    void restoreCurrentToValueBeforeBalancing();

    void doSetVoltage(float value);
    void doSetCurrent(float value);

    void setCcMode(bool cc_mode);
    void setCvMode(bool cv_mode);
    void updateCcAndCvSwitch();
    
    void executeOutputEnable(bool enable);
    void doOutputEnable(bool enable);
    
    void doRemoteSensingEnable(bool enable);
    void doRemoteProgrammingEnable(bool enable);

    void lowRippleCheck(uint32_t tick_usec);
    void doLowRippleEnable(bool enable);
    void doLowRippleAutoEnable(bool enable);

    void doDpEnable(bool enable);

#if !CONF_SKIP_PWRGOOD_TEST
    void testPwrgood(uint8_t gpio);
#endif

    float getDualRangeGndOffset();
    //void calculateNegligibleAdcDiffForCurrent();

    uint32_t autoRangeCheckLastTickCount;
    void doAutoSelectCurrentRange(uint32_t tickCount);

    void doSetCurrentRange();
};

}
} // namespace eez::psu