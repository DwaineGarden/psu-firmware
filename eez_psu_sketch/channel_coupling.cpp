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
#include "channel_coupling.h"
#include "bp.h"

namespace eez {
namespace psu {
namespace channel_coupling {

static Type g_channelCoupling = TYPE_NONE;

void setType(Type value) {
    if (g_channelCoupling != value) {
        if (CH_NUM < 2) {
            return;
        }

        g_channelCoupling = value;

        for (int i = 0; i < 2; ++i) {
    		Channel &channel = Channel::get(i);
            channel.outputEnable(false);
            if (channel.getFeatures() & CH_FEATURE_RPROG) {
                channel.remoteProgrammingEnable(false);
            }
            if (channel.getFeatures() & CH_FEATURE_LRIPPLE) {
                channel.lowRippleEnable(false);
            }

            channel.setVoltage(getUMin(channel));
            channel.setCurrent(getIMin(channel));
        }

        bp::switchChannelCoupling(g_channelCoupling);
    }
}

Type getType() {
    return g_channelCoupling;
}

float getUSet(const Channel &channel) { 
    if (g_channelCoupling == TYPE_SERIES) {
        return Channel::get(0).u.set + Channel::get(1).u.set;
    }
    return channel.u.set;
}

float getUMon(const Channel &channel) { 
    if (g_channelCoupling == TYPE_SERIES) {
        return Channel::get(0).u.mon + Channel::get(1).u.mon;
    }
    return channel.u.mon; 
}

float getUMonDac(const Channel &channel) { 
    if (g_channelCoupling == TYPE_SERIES) {
        return Channel::get(0).u.mon_dac + Channel::get(1).u.mon_dac;
    }
    return channel.u.mon_dac; 
}

float getULimit(const Channel &channel) {
    if (g_channelCoupling == TYPE_SERIES) {
        return 2 * min(Channel::get(0).getVoltageLimit(), Channel::get(1).getVoltageLimit());
    }
    return channel.getVoltageLimit();
}

float getUMaxLimit(const Channel &channel) {
    if (g_channelCoupling == TYPE_SERIES) {
        return 2 * min(Channel::get(0).getVoltageMaxLimit(), Channel::get(1).getVoltageMaxLimit());
    }
    return channel.getVoltageMaxLimit();
}

float getUMin(const Channel &channel) {
    if (g_channelCoupling == TYPE_SERIES) {
        return 2 * max(Channel::get(0).u.min, Channel::get(1).u.min);
    }
    return channel.u.min;
}

float getUDef(const Channel &channel) {
    if (g_channelCoupling == TYPE_SERIES) {
        return Channel::get(0).u.def + Channel::get(1).u.def;
    }
    return channel.u.def;
}

float getUMax(const Channel &channel) {
    if (g_channelCoupling == TYPE_SERIES) {
        return 2 * min(Channel::get(0).u.max, Channel::get(1).u.max);
    }
    return channel.u.max;
}

void setVoltage(Channel &channel, float voltage) {
    if (g_channelCoupling == TYPE_SERIES) {
        Channel::get(0).setVoltage(voltage / 2);
        Channel::get(1).setVoltage(voltage / 2);
    } else if (g_channelCoupling == TYPE_PARALLEL) {
        Channel::get(0).setVoltage(voltage);
        Channel::get(1).setVoltage(voltage);
    } else {
        channel.setVoltage(voltage);
    }
}

void setVoltageLimit(Channel &channel, float limit) {
    if (g_channelCoupling == TYPE_SERIES) {
        Channel::get(0).setVoltageLimit(limit / 2);
        Channel::get(1).setVoltageLimit(limit / 2);
    } else if (g_channelCoupling == TYPE_PARALLEL) {
        Channel::get(0).setVoltageLimit(limit);
        Channel::get(1).setVoltageLimit(limit);
    } else {
        channel.setVoltageLimit(limit);
    }
}

float getISet(const Channel &channel) { 
    if (g_channelCoupling == TYPE_PARALLEL) {
        return Channel::get(0).i.set + Channel::get(1).i.set;
    }
    return channel.i.set; 
}

float getIMon(const Channel &channel) { 
    if (g_channelCoupling == TYPE_PARALLEL) {
        return Channel::get(0).i.mon + Channel::get(1).i.mon;
    }
    return channel.i.mon; 
}

float getIMonDac(const Channel &channel) { 
    if (g_channelCoupling == TYPE_PARALLEL) {
        return Channel::get(0).i.mon_dac + Channel::get(1).i.mon_dac;
    }
    return channel.i.mon_dac; 
}

float getILimit(const Channel &channel) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        return 2 * min(Channel::get(0).getCurrentLimit(), Channel::get(1).getCurrentLimit());
    }
    return channel.getCurrentLimit();
}

float getIMaxLimit(const Channel &channel) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        return 2 * min(Channel::get(0).getMaxCurrentLimit(), Channel::get(1).getMaxCurrentLimit());
    }
    return channel.getMaxCurrentLimit();
}

float getIMin(const Channel &channel) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        return 2 * max(Channel::get(0).i.min, Channel::get(1).i.min);
    }
    return channel.i.min;
}

float getIDef(const Channel &channel) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        return Channel::get(0).i.def + Channel::get(1).i.def;
    }
    return channel.i.def;
}

float getIMax(const Channel &channel) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        return 2 * min(Channel::get(0).i.max, Channel::get(1).i.max);
    }
    return channel.i.max;
}

void setCurrent(Channel &channel, float current) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        Channel::get(0).setCurrent(current / 2);
        Channel::get(1).setCurrent(current / 2);
    } else if (g_channelCoupling == TYPE_SERIES) {
        Channel::get(0).setCurrent(current);
        Channel::get(1).setCurrent(current);
    } else {
        channel.setCurrent(current);
    }
}

void setCurrentLimit(Channel &channel, float limit) {
    if (g_channelCoupling == TYPE_PARALLEL) {
        Channel::get(0).setCurrentLimit(limit / 2);
        Channel::get(1).setCurrentLimit(limit / 2);
    } else if (g_channelCoupling == TYPE_SERIES) {
        Channel::get(0).setCurrentLimit(limit);
        Channel::get(1).setCurrentLimit(limit);
    } else {
        channel.setCurrentLimit(limit);
    }
}

float getPowerLimit(const Channel& channel) {
    if (g_channelCoupling != TYPE_NONE) {
        return 2 * min(Channel::get(0).getPowerLimit(), Channel::get(1).getPowerLimit());
    }
    return channel.getPowerLimit();
}

float getPowerMinLimit(const Channel& channel) {
    if (g_channelCoupling != TYPE_NONE) {
        return 0;
    }
    return 0;
}

float getPowerMaxLimit(const Channel& channel) {
    if (g_channelCoupling != TYPE_NONE) {
        return 2 * min(Channel::get(0).PTOT, Channel::get(1).PTOT);
    }
    return channel.PTOT;
}

float getPowerDefaultLimit(const Channel& channel) {
    return getPowerMaxLimit(channel);
}

void setPowerLimit(Channel &channel, float limit) {
    if (g_channelCoupling != TYPE_NONE) {
        Channel::get(0).setPowerLimit(limit / 2);
        Channel::get(1).setPowerLimit(limit / 2);
    } else {
        channel.setPowerLimit(limit);
    }
}

void outputEnable(Channel& channel, bool enable) {
    if (g_channelCoupling != TYPE_NONE) {
        Channel::get(0).outputEnable(enable);
        Channel::get(1).outputEnable(enable);
    } else {
        channel.outputEnable(enable);
    }
}

bool isLowRippleAllowed(Channel& channel) {
    if (g_channelCoupling != TYPE_NONE) {
        return (Channel::get(0).getFeatures() & CH_FEATURE_LRIPPLE) && (Channel::get(1).getFeatures() & CH_FEATURE_LRIPPLE);
    } else {
        return (channel.getFeatures() & CH_FEATURE_LRIPPLE) ? true : false;
    }
}

bool lowRippleEnable(Channel& channel, bool enable) {
    if (g_channelCoupling != TYPE_NONE) {
        return Channel::get(0).lowRippleEnable(enable) && Channel::get(1).lowRippleEnable(enable);
    } else {
        return channel.lowRippleEnable(enable);
    }
}

void lowRippleAutoEnable(Channel& channel, bool enable) {
    if (g_channelCoupling != TYPE_NONE) {
        Channel::get(0).lowRippleAutoEnable(enable);
        Channel::get(1).lowRippleAutoEnable(enable);
    } else {
        channel.outputEnable(enable);
    }
}

}
}
} // namespace eez::psu::channel_coupling