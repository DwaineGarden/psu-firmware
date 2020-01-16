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
 
#include "psu.h"
#include "channel_dispatcher.h"
#include "bp.h"
#include "temperature.h"
#include "event_queue.h"
#include "trigger.h"
#include "list.h"
#include "scpi_regs.h"

namespace eez {
namespace psu {
namespace channel_dispatcher {

static Type g_channelCoupling = TYPE_NONE;

bool isCouplingOrTrackingAllowed() {
    if (CH_NUM < 2) {
        return false;
    }

    if (!Channel::get(0).isOk() || !Channel::get(1).isOk()) {
        return false;
    }

    return true;
}

bool setType(Type value) {
    if (g_channelCoupling != value) {
        if (value != TYPE_NONE && !isCouplingOrTrackingAllowed()) {
            return false;
        }

        g_channelCoupling = value;

        for (int i = 0; i < 2; ++i) {
            if (i < CH_NUM) {
                Channel &channel = Channel::get(i);
                if (Channel::get(i).isOk()) {
                    channel.outputEnable(false);
                    channel.remoteSensingEnable(false);

                    if (channel.getFeatures() & CH_FEATURE_RPROG) {
                        channel.remoteProgrammingEnable(false);
                    }
                    if (channel.getFeatures() & CH_FEATURE_LRIPPLE) {
                        channel.lowRippleEnable(false);
                        channel.lowRippleAutoEnable(false);
                    }

                    channel.setVoltageTriggerMode(TRIGGER_MODE_FIXED);
                    channel.setCurrentTriggerMode(TRIGGER_MODE_FIXED);
                    channel.setTriggerOutputState(true);
                    channel.setTriggerOnListStop(TRIGGER_ON_LIST_STOP_OUTPUT_OFF);

                    list::resetChannelList(channel);

                    if (isTracked()) {
                        channel.setVoltageLimit(MIN(Channel::get(0).getVoltageLimit(), Channel::get(1).getVoltageLimit()));
                        if (i != 0) {
                            channel.setVoltage(Channel::get(0).u.set);
                        }

                        channel.setCurrentLimit(MIN(Channel::get(0).getCurrentLimit(), Channel::get(1).getCurrentLimit()));
                        if (i != 0) {
                            channel.setCurrent(Channel::get(0).i.set);
                        }

                        trigger::setVoltage(channel, Channel::get(0).u.def);
                        trigger::setCurrent(channel, Channel::get(0).i.def);
                    } else {
                        channel.setVoltage(getUMin(channel));
                        channel.setVoltageLimit(MIN(Channel::get(0).getVoltageLimit(), Channel::get(1).getVoltageLimit()));

                        channel.setCurrent(getIMin(channel));
                        channel.setCurrentLimit(MIN(Channel::get(0).getCurrentLimit(), Channel::get(1).getCurrentLimit()));

                        trigger::setVoltage(channel, getUMin(channel));
                        trigger::setCurrent(channel, getIMin(channel));

#ifdef EEZ_PSU_SIMULATOR
                        channel.simulator.setLoadEnabled(false);
                        channel.simulator.setLoad(Channel::get(0).simulator.getLoad());
#endif
                    }

                    if (isTracked() || isCoupled()) {
                        channel.prot_conf.flags.u_state = Channel::get(0).prot_conf.flags.u_state || Channel::get(1).prot_conf.flags.u_state ? 1 : 0;
                        channel.prot_conf.u_level = MIN(Channel::get(0).prot_conf.u_level, Channel::get(1).prot_conf.u_level);
                        channel.prot_conf.u_delay = MIN(Channel::get(0).prot_conf.u_delay, Channel::get(1).prot_conf.u_delay);

                        channel.prot_conf.flags.i_state = Channel::get(0).prot_conf.flags.i_state || Channel::get(1).prot_conf.flags.i_state ? 1 : 0;
                        channel.prot_conf.i_delay = MIN(Channel::get(0).prot_conf.i_delay, Channel::get(1).prot_conf.i_delay);

                        channel.prot_conf.flags.p_state = Channel::get(0).prot_conf.flags.p_state || Channel::get(1).prot_conf.flags.p_state ? 1 : 0;
                        channel.prot_conf.p_level = MIN(Channel::get(0).prot_conf.p_level, Channel::get(1).prot_conf.p_level);
                        channel.prot_conf.p_delay = MIN(Channel::get(0).prot_conf.p_delay, Channel::get(1).prot_conf.p_delay);

                        temperature::sensors[temp_sensor::CH1 + channel.index - 1].prot_conf.state =
                            temperature::sensors[temp_sensor::CH1].prot_conf.state ||
                            temperature::sensors[temp_sensor::CH2].prot_conf.state ? 1 : 0;

                        temperature::sensors[temp_sensor::CH1 + channel.index - 1].prot_conf.level =
                            MIN(temperature::sensors[temp_sensor::CH1].prot_conf.level,
                                temperature::sensors[temp_sensor::CH2].prot_conf.level);

                        temperature::sensors[temp_sensor::CH1 + channel.index - 1].prot_conf.delay =
                            MIN(temperature::sensors[temp_sensor::CH1].prot_conf.delay,
                                temperature::sensors[temp_sensor::CH2].prot_conf.delay);
                    }
                }

                if (i == 1) {
                    Channel &channel1 = Channel::get(0);
                    channel.flags.displayValue1 = channel1.flags.displayValue1;
                    channel.flags.displayValue2 = channel1.flags.displayValue2;
                    channel.ytViewRate = channel1.ytViewRate;

                    if (isCoupled() || isTracked()) {
                        channel.setVoltageTriggerMode(TRIGGER_MODE_FIXED);
                        channel.setCurrentTriggerMode(TRIGGER_MODE_FIXED);
                        channel.setTriggerOutputState(true);
                        channel.setTriggerOnListStop(TRIGGER_ON_LIST_STOP_OUTPUT_OFF);
                    }
                }

                channel.setCurrentRangeSelectionMode(CURRENT_RANGE_SELECTION_USE_BOTH);
                channel.enableAutoSelectCurrentRange(false);

                channel.resetHistory();
            }
        }

        bp::switchChannelCoupling(g_channelCoupling);

        if (isParallel()) {
            event_queue::pushEvent(event_queue::EVENT_INFO_COUPLED_IN_PARALLEL);
        } else if (isSeries()) {
            event_queue::pushEvent(event_queue::EVENT_INFO_COUPLED_IN_SERIES);
        } else if (isTracked()) {
            event_queue::pushEvent(event_queue::EVENT_INFO_CHANNELS_TRACKED);
        } else {
            event_queue::pushEvent(event_queue::EVENT_INFO_CHANNELS_UNCOUPLED);
        }

        psu::setOperBits(OPER_GROUP_PARALLEL, isParallel());
        psu::setOperBits(OPER_GROUP_SERIAL, isSeries());

        delay(100); // Huge pause that allows relay contacts to debounce
    }

    return true;
}

Type getType() {
    return g_channelCoupling;
}

float getUSet(const Channel &channel) { 
    if (isSeries()) {
        return Channel::get(0).u.set + Channel::get(1).u.set;
    }
    return channel.u.set;
}

float getUSetUnbalanced(const Channel &channel) { 
    if (isSeries()) {
        return Channel::get(0).getUSetUnbalanced() + Channel::get(1).getUSetUnbalanced();
    }
    return channel.u.set;
}

float getUMon(const Channel &channel) { 
    if (isSeries()) {
        return Channel::get(0).u.mon + Channel::get(1).u.mon;
    }
    return channel.u.mon; 
}

float getUMonLast(const Channel &channel) {
	if (isSeries()) {
		return Channel::get(0).u.mon_last + Channel::get(1).u.mon_last;
	}
	return channel.u.mon_last;
}

float getUMonHistory(const Channel &channel, int position) {
    if (isSeries()) {
        return Channel::get(0).getUMonHistory(position) + Channel::get(1).getUMonHistory(position);
    }
    return channel.getUMonHistory(position); 
}

float getUMonDac(const Channel &channel) { 
    if (isSeries()) {
        return Channel::get(0).u.mon_dac + Channel::get(1).u.mon_dac;
    }
    return channel.u.mon_dac; 
}

float getULimit(const Channel &channel) {
    if (isSeries()) {
        return 2 * MIN(Channel::get(0).getVoltageLimit(), Channel::get(1).getVoltageLimit());
    }
    return channel.getVoltageLimit();
}

float getUMaxLimit(const Channel &channel) {
    if (isSeries()) {
        return 2 * MIN(Channel::get(0).getVoltageMaxLimit(), Channel::get(1).getVoltageMaxLimit());
    } else if (isParallel() || isTracked()) {
        return MIN(Channel::get(0).getVoltageMaxLimit(), Channel::get(1).getVoltageMaxLimit());
    }
    return channel.getVoltageMaxLimit();
}

float getUMin(const Channel &channel) {
    if (isSeries()) {
        return 2 * MAX(Channel::get(0).u.min, Channel::get(1).u.min);
    } else if (isParallel() || isTracked()) {
        return MAX(Channel::get(0).u.min, Channel::get(1).u.min);
    }
    return channel.u.min;
}

float getUDef(const Channel &channel) {
    if (isSeries()) {
        return Channel::get(0).u.def + Channel::get(1).u.def;
    }
    return channel.u.def;
}

float getUMax(const Channel &channel) {
    if (isSeries()) {
        return 2 * MIN(Channel::get(0).u.max, Channel::get(1).u.max);
    } else if (isParallel() || isTracked()) {
        return MIN(Channel::get(0).u.max, Channel::get(1).u.max);
    }
    return channel.u.max;
}

float getUProtectionLevel(const Channel &channel) {
    if (isSeries()) {
        return Channel::get(0).prot_conf.u_level + Channel::get(1).prot_conf.u_level;
    }
    return channel.prot_conf.u_level;
}

void setVoltage(Channel &channel, float voltage) {
    if (isSeries()) {
        Channel::get(0).setVoltage(voltage / 2);
        Channel::get(1).setVoltage(voltage / 2);
    } else if (isParallel() || isTracked()) {
        Channel::get(0).setVoltage(voltage);
        Channel::get(1).setVoltage(voltage);
    } else {
        channel.setVoltage(voltage);
    }
}

void setVoltageLimit(Channel &channel, float limit) {
    if (isSeries()) {
        Channel::get(0).setVoltageLimit(limit / 2);
        Channel::get(1).setVoltageLimit(limit / 2);
    } else if (isParallel() || isTracked()) {
        Channel::get(0).setVoltageLimit(limit);
        Channel::get(1).setVoltageLimit(limit);
    } else {
        channel.setVoltageLimit(limit);
    }
}

void setOvpParameters(Channel &channel, int state, float level, float delay) {
    if (isCoupled() || isTracked()) {
        float coupledLevel = isSeries() ? level / 2 : level;

        Channel::get(0).prot_conf.flags.u_state = state;
        Channel::get(0).prot_conf.u_level = coupledLevel;
        Channel::get(0).prot_conf.u_delay = delay;

        Channel::get(1).prot_conf.flags.u_state = state;
        Channel::get(1).prot_conf.u_level = coupledLevel;
        Channel::get(1).prot_conf.u_delay = delay;
    } else {
        channel.prot_conf.flags.u_state = state;
        channel.prot_conf.u_level = level;
        channel.prot_conf.u_delay = delay;
    }
}

void setOvpState(Channel &channel, int state) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.flags.u_state = state;
        Channel::get(1).prot_conf.flags.u_state = state;
    } else {
        channel.prot_conf.flags.u_state = state;
    }
}

void setOvpLevel(Channel &channel, float level) {
    if (isCoupled() || isTracked()) {
        float coupledLevel = isSeries() ? level / 2 : level;
        Channel::get(0).prot_conf.u_level = coupledLevel;
        Channel::get(1).prot_conf.u_level = coupledLevel;
    } else {
        channel.prot_conf.u_level = level;
    }
}

void setOvpDelay(Channel &channel, float delay) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.u_delay = delay;
        Channel::get(1).prot_conf.u_delay = delay;
    } else {
        channel.prot_conf.u_delay = delay;
    }
}

float getISet(const Channel &channel) { 
    if (isParallel()) {
        return Channel::get(0).i.set + Channel::get(1).i.set;
    }
    return channel.i.set; 
}

float getISetUnbalanced(const Channel &channel) { 
    if (isParallel()) {
        return Channel::get(0).getISetUnbalanced() + Channel::get(1).getISetUnbalanced();
    }
    return channel.i.set; 
}

float getIMon(const Channel &channel) { 
    if (isParallel()) {
        return Channel::get(0).i.mon + Channel::get(1).i.mon;
    }
    return channel.i.mon; 
}

float getIMonLast(const Channel &channel) {
	if (isParallel()) {
		return Channel::get(0).i.mon_last + Channel::get(1).i.mon_last;
	}
	return channel.i.mon_last;
}

float getIMonHistory(const Channel &channel, int position) {
    if (isParallel()) {
        return Channel::get(0).getIMonHistory(position) + Channel::get(1).getIMonHistory(position);
    }
    return channel.getIMonHistory(position); 
}

float getIMonDac(const Channel &channel) { 
    if (isParallel()) {
        return Channel::get(0).i.mon_dac + Channel::get(1).i.mon_dac;
    }
    return channel.i.mon_dac; 
}

float getILimit(const Channel &channel) {
    if (isParallel()) {
        return 2 * MIN(Channel::get(0).getCurrentLimit(), Channel::get(1).getCurrentLimit());
    }
    return channel.getCurrentLimit();
}

float getIMaxLimit(const Channel &channel) {
    if (isParallel()) {
        return 2 * MIN(Channel::get(0).getMaxCurrentLimit(), Channel::get(1).getMaxCurrentLimit());
    } else if (isSeries() || isTracked()) {
        return MIN(Channel::get(0).getMaxCurrentLimit(), Channel::get(1).getMaxCurrentLimit());
    }
    return channel.getMaxCurrentLimit();
}

float getIMin(const Channel &channel) {
    if (isParallel()) {
        return 2 * MAX(Channel::get(0).i.min, Channel::get(1).i.min);
    } else if (isSeries() || isTracked()) {
        return MAX(Channel::get(0).i.min, Channel::get(1).i.min);
    }
    return channel.i.min;
}

float getIDef(const Channel &channel) {
    if (isParallel()) {
        return Channel::get(0).i.def + Channel::get(1).i.def;
    }
    return channel.i.def;
}

float getIMax(const Channel &channel) {
    if (isParallel()) {
        return 2 * MIN(Channel::get(0).i.max, Channel::get(1).i.max);
    } else if (isSeries() || isTracked()) {
        return MIN(Channel::get(0).i.max, Channel::get(1).i.max);
    }
    return channel.i.max;
}

void setCurrent(Channel &channel, float current) {
    if (isParallel()) {
        Channel::get(0).setCurrent(current / 2);
        Channel::get(1).setCurrent(current / 2);
    } else if (isSeries() || isTracked()) {
        Channel::get(0).setCurrent(current);
        Channel::get(1).setCurrent(current);
    } else {
        channel.setCurrent(current);
    }
}

void setCurrentLimit(Channel &channel, float limit) {
    if (isParallel()) {
        Channel::get(0).setCurrentLimit(limit / 2);
        Channel::get(1).setCurrentLimit(limit / 2);
    } else if (isSeries() || isTracked()) {
        Channel::get(0).setCurrentLimit(limit);
        Channel::get(1).setCurrentLimit(limit);
    } else {
        channel.setCurrentLimit(limit);
    }
}

void setOcpParameters(Channel &channel, int state, float delay) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.flags.i_state = state;
        Channel::get(0).prot_conf.i_delay = delay;

        Channel::get(1).prot_conf.flags.i_state = state;
        Channel::get(1).prot_conf.i_delay = delay;
    } else {
        channel.prot_conf.flags.i_state = state;
        channel.prot_conf.i_delay = delay;
    }
}

void setOcpState(Channel &channel, int state) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.flags.i_state = state;
        Channel::get(1).prot_conf.flags.i_state = state;
    } else {
        channel.prot_conf.flags.i_state = state;
    }
}

void setOcpDelay(Channel &channel, float delay) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.i_delay = delay;
        Channel::get(1).prot_conf.i_delay = delay;
    } else {
        channel.prot_conf.i_delay = delay;
    }
}

float getPowerLimit(const Channel& channel) {
    if (isCoupled()) {
        return 2 * MIN(Channel::get(0).getPowerLimit(), Channel::get(1).getPowerLimit());
    }
    return channel.getPowerLimit();
}

float getPowerMinLimit(const Channel& channel) {
    return 0;
}

float getPowerMaxLimit(const Channel& channel) {
    if (isCoupled()) {
        return 2 * MIN(Channel::get(0).PTOT, Channel::get(1).PTOT);
    }
    return channel.PTOT;
}

float getPowerDefaultLimit(const Channel& channel) {
    return getPowerMaxLimit(channel);
}

float getPowerProtectionLevel(const Channel &channel) {
    if (isCoupled()) {
        return Channel::get(0).prot_conf.p_level + Channel::get(1).prot_conf.p_level;
    }
    return channel.prot_conf.p_level;
}

void setPowerLimit(Channel &channel, float limit) {
    if (isCoupled()) {
        Channel::get(0).setPowerLimit(limit / 2);
        Channel::get(1).setPowerLimit(limit / 2);
    } else if (isTracked()) {
        Channel::get(0).setPowerLimit(limit);
        Channel::get(1).setPowerLimit(limit);
    } else {
        channel.setPowerLimit(limit);
    }
}

float getOppMinLevel(Channel &channel) {
    if (isCoupled()) {
        return 2 * MAX(Channel::get(0).OPP_MIN_LEVEL, Channel::get(1).OPP_MIN_LEVEL);
    }
    return channel.OPP_MIN_LEVEL;
}

float getOppMaxLevel(Channel &channel) {
    if (isCoupled()) {
        return 2 * MIN(Channel::get(0).OPP_MAX_LEVEL, Channel::get(1).OPP_MAX_LEVEL);
    }
    return channel.OPP_MAX_LEVEL;
}

float getOppDefaultLevel(Channel &channel) {
    if (isCoupled()) {
        return Channel::get(0).OPP_DEFAULT_LEVEL + Channel::get(1).OPP_DEFAULT_LEVEL;
    }
    return channel.OPP_DEFAULT_LEVEL;
}

void setOppParameters(Channel &channel, int state, float level, float delay) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.flags.p_state = state;
        Channel::get(0).prot_conf.p_level = isCoupled() ? level / 2 : level;
        Channel::get(0).prot_conf.p_delay = delay;

        Channel::get(1).prot_conf.flags.p_state = state;
        Channel::get(1).prot_conf.p_level = isCoupled() ? level / 2 : level;
        Channel::get(1).prot_conf.p_delay = delay;
    } else {
        channel.prot_conf.flags.p_state = state;
        channel.prot_conf.p_level = level;
        channel.prot_conf.p_delay = delay;
    }
}

void setOppState(Channel &channel, int state) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.flags.p_state = state;
        Channel::get(1).prot_conf.flags.p_state = state;
    } else {
        channel.prot_conf.flags.p_state = state;
    }
}

void setOppLevel(Channel &channel, float level) {
    if (isCoupled()) {
        Channel::get(0).prot_conf.p_level = level / 2;
        Channel::get(1).prot_conf.p_level = level / 2;
    } else if (isTracked()) {
        Channel::get(0).prot_conf.p_level = level;
        Channel::get(1).prot_conf.p_level = level;
    } else {
        channel.prot_conf.p_level = level;
    }
}

void setOppDelay(Channel &channel, float delay) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).prot_conf.p_delay = delay;
        Channel::get(1).prot_conf.p_delay = delay;
    } else {
        channel.prot_conf.p_delay = delay;
    }
}

void outputEnable(Channel& channel, bool enable) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).outputEnable(enable);
        Channel::get(1).outputEnable(enable);
    } else {
        channel.outputEnable(enable);
    }
}

void remoteSensingEnable(Channel& channel, bool enable) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).remoteSensingEnable(enable);
        Channel::get(1).remoteSensingEnable(enable);
    } else {
        channel.remoteSensingEnable(enable);
    }
}

void remoteProgrammingEnable(Channel& channel, bool enable) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).remoteSensingEnable(enable);
        Channel::get(1).remoteSensingEnable(enable);
    } else {
        channel.remoteSensingEnable(enable);
    }
}

bool lowRippleEnable(Channel& channel, bool enable) {
    if (isCoupled() || isTracked()) {
        bool success = Channel::get(0).lowRippleEnable(enable) && Channel::get(1).lowRippleEnable(enable);
        if (!success) {
            Channel::get(0).lowRippleEnable(false);
            Channel::get(1).lowRippleEnable(false);
        }
        return success;
    } else {
        return channel.lowRippleEnable(enable);
    }
}

void lowRippleAutoEnable(Channel& channel, bool enable) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).lowRippleAutoEnable(enable);
        Channel::get(1).lowRippleAutoEnable(enable);
    } else {
        channel.lowRippleAutoEnable(enable);
    }
}

bool isTripped(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).isTripped() || Channel::get(1).isTripped();
    } else {
        return channel.isTripped();
    }
}

void clearProtection(Channel& channel) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).clearProtection();
        Channel::get(1).clearProtection();
    } else {
        channel.clearProtection();
    }
}

void disableProtection(Channel& channel) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).disableProtection();
        Channel::get(1).disableProtection();
    } else {
        channel.disableProtection();
    }
}

bool isOvpTripped(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).ovp.flags.tripped || Channel::get(1).ovp.flags.tripped;
    } else {
        return channel.ovp.flags.tripped;
    }
}

bool isOcpTripped(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).ocp.flags.tripped || Channel::get(1).ocp.flags.tripped;
    } else {
        return channel.ocp.flags.tripped;
    }
}

bool isOppTripped(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).opp.flags.tripped || Channel::get(1).opp.flags.tripped;
    } else {
        return channel.opp.flags.tripped;
    }
}

bool isOtpTripped(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return temperature::sensors[temp_sensor::CH1].isTripped() || temperature::sensors[temp_sensor::CH2].isTripped();
    } else {
        return temperature::sensors[temp_sensor::CH1 + channel.index - 1].isTripped();
    }
}

void clearOtpProtection(int sensor) {
    if ((sensor == temp_sensor::CH1 || sensor == temp_sensor::CH2) && (isCoupled() || isTracked())) {
        temperature::sensors[temp_sensor::CH1].clearProtection();
        temperature::sensors[temp_sensor::CH2].clearProtection();
    } else {
        temperature::sensors[sensor].clearProtection();
    }
}

void setOtpParameters(Channel &channel, int state, float level, float delay) {
    if (isCoupled() || isTracked()) {
        temperature::sensors[temp_sensor::CH1].prot_conf.state = state ? true : false;
        temperature::sensors[temp_sensor::CH2].prot_conf.state = state ? true : false;

        temperature::sensors[temp_sensor::CH1].prot_conf.level = level;
        temperature::sensors[temp_sensor::CH2].prot_conf.level = level;

        temperature::sensors[temp_sensor::CH1].prot_conf.delay = delay;
        temperature::sensors[temp_sensor::CH2].prot_conf.delay = delay;
    } else {
        temperature::sensors[temp_sensor::CH1 + channel.index - 1].prot_conf.state = state ? true : false;
        temperature::sensors[temp_sensor::CH1 + channel.index - 1].prot_conf.level = level;
        temperature::sensors[temp_sensor::CH1 + channel.index - 1].prot_conf.delay = delay;
    }
}

void setOtpState(int sensor, int state) {
    if ((sensor == temp_sensor::CH1 || sensor == temp_sensor::CH2) && (isCoupled() || isTracked())) {
        temperature::sensors[temp_sensor::CH1].prot_conf.state = state ? true : false;
        temperature::sensors[temp_sensor::CH2].prot_conf.state = state ? true : false;
    } else {
        temperature::sensors[sensor].prot_conf.state = state ? true : false;
    }
}

void setOtpLevel(int sensor, float level) {
    if ((sensor == temp_sensor::CH1 || sensor == temp_sensor::CH2) && (isCoupled() || isTracked())) {
        temperature::sensors[temp_sensor::CH1].prot_conf.level = level;
        temperature::sensors[temp_sensor::CH2].prot_conf.level = level;
    } else {
        temperature::sensors[sensor].prot_conf.level = level;
    }
}

void setOtpDelay(int sensor, float delay) {
    if ((sensor == temp_sensor::CH1 || sensor == temp_sensor::CH2) && (isCoupled() || isTracked())) {
        temperature::sensors[temp_sensor::CH1].prot_conf.delay = delay;
        temperature::sensors[temp_sensor::CH2].prot_conf.delay = delay;
    } else {
        temperature::sensors[sensor].prot_conf.delay = delay;
    }
}

void setDisplayViewSettings(Channel &channel, int displayValue1, int displayValue2, float ytViewRate) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).flags.displayValue1 = displayValue1;
        Channel::get(0).flags.displayValue2 = displayValue2;
        Channel::get(0).ytViewRate = ytViewRate;

        Channel::get(1).flags.displayValue1 = displayValue1;
        Channel::get(1).flags.displayValue2 = displayValue2;
        Channel::get(1).ytViewRate = ytViewRate;
    } else {
        channel.flags.displayValue1 = displayValue1;
        channel.flags.displayValue2 = displayValue2;
        channel.ytViewRate = ytViewRate;
    }
}

TriggerMode getVoltageTriggerMode(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).getVoltageTriggerMode();
    } else {
        return channel.getVoltageTriggerMode();
    }
}

void setVoltageTriggerMode(Channel& channel, TriggerMode mode) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).setVoltageTriggerMode(mode);
    } else {
        channel.setVoltageTriggerMode(mode);
    }
}

TriggerMode getCurrentTriggerMode(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).getCurrentTriggerMode();
    } else {
        return channel.getCurrentTriggerMode();
    }
}

void setCurrentTriggerMode(Channel& channel, TriggerMode mode) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).setCurrentTriggerMode(mode);
    } else {
        channel.setCurrentTriggerMode(mode);
    }
}

bool getTriggerOutputState(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).getTriggerOutputState();
    } else {
        return channel.getTriggerOutputState();
    }
}

void setTriggerOutputState(Channel& channel, bool enable) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).setTriggerOutputState(enable);
    } else {
        channel.setTriggerOutputState(enable);
    }
}

TriggerOnListStop getTriggerOnListStop(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).getTriggerOnListStop();
    } else {
        return channel.getTriggerOnListStop();
    }
}

void setTriggerOnListStop(Channel& channel, TriggerOnListStop value) {
    if (isCoupled() || isTracked()) {
        Channel::get(0).setTriggerOnListStop(value);
    } else {
        channel.setTriggerOnListStop(value);
    }
}

float getTriggerVoltage(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return trigger::getVoltage(Channel::get(0));
    } else {
        return trigger::getVoltage(channel);
    }
}

void setTriggerVoltage(Channel& channel, float value) {
    if (isCoupled() || isTracked()) {
        trigger::setVoltage(Channel::get(0), value);
    } else {
        trigger::setVoltage(channel, value);
    }
}

float getTriggerCurrent(Channel& channel) {
    if (isCoupled() || isTracked()) {
        return trigger::getCurrent(Channel::get(0));
    } else {
        return trigger::getCurrent(channel);
    }
}

void setTriggerCurrent(Channel& channel, float value) {
    if (isCoupled() || isTracked()) {
        trigger::setCurrent(Channel::get(0), value);
    } else {
        trigger::setCurrent(channel, value);
    }
}

#ifdef EEZ_PSU_SIMULATOR
void setLoadEnabled(Channel &channel, bool state) {
    if (isCoupled()) {
        Channel::get(0).simulator.setLoadEnabled(state);
        Channel::get(1).simulator.setLoadEnabled(state);
    } else {
        channel.simulator.setLoadEnabled(state);
    }
}

void setLoad(Channel &channel, float load) {
    if (isCoupled()) {
        Channel::get(0).simulator.setLoad(load);
        Channel::get(1).simulator.setLoad(load);
    } else {
        channel.simulator.setLoad(load);
    }
}
#endif

bool isCurrentLowRangeAllowed(Channel &channel) {
    if (isCoupled() || isTracked()) {
        return Channel::get(0).isCurrentLowRangeAllowed() || Channel::get(1).isCurrentLowRangeAllowed();
    } else {
        return channel.isCurrentLowRangeAllowed();
    }
}

}
}
} // namespace eez::psu::channel_dispatcher