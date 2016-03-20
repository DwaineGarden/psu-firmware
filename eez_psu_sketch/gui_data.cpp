/*
 * EEZ PSU Firmware
 * Copyright (C) 2015 Envox d.o.o.
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
#include "gui_internal.h"
#include "gui_keypad.h"
#include "channel.h"

namespace eez {
namespace psu {
namespace gui {
namespace data {

void Value::toText(char *text, int count) {
    text[0] = 0;

    util::strcatFloat(text, float_);

    switch (unit_) {
    case UNIT_VOLT:
        strcat(text, " V");
        break;
    
    case UNIT_AMPER:
        strcat(text, " A");
        break;

    case UNIT_CONST_STR:
        strncpy_P(text, const_str_, count - 1);
        text[count - 1] = 0;
        break;

    case UNIT_STR:
        strncpy(text, str_, count - 1);
        text[count - 1] = 0;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////

static Cursor cursor;
static Value alert_message;
static Unit last_edit_unit;
static char edit_info[32];
static char last_edit_info[32];
static float last_edit_value;

Cursor getCursor() {
    return cursor;
}

void setCursor(Cursor cursor_) {
    cursor = cursor_;
}

////////////////////////////////////////////////////////////////////////////////

int count(uint8_t id) {
    if (id == DATA_ID_CHANNELS) {
        return CH_NUM;
    }
    return 0;
}

void select(uint8_t id, int index) {
    if (id == DATA_ID_CHANNELS) {
        cursor.selected_channel_index = index;
    }
}

Value get(uint8_t id, bool &changed) {
    Channel *selected_channel = &Channel::get(cursor.selected_channel_index);
    ChannelState *selected_channel_last_state = &cursor.channel_last_state[cursor.selected_channel_index];

    Value value;

    changed = false;

    if (id == DATA_ID_OUTPUT_STATE) {
        uint8_t state = selected_channel->isOutputEnabled() ? 1 : 0;
        if (state != selected_channel_last_state->flags.state) {
            selected_channel_last_state->flags.state = state;
            changed = true;
        }
        value = Value(state);
    } else if (id == DATA_ID_OUTPUT_MODE) {
        char *mode_str = selected_channel->getCvModeStr();
        uint8_t mode = strcmp(mode_str, "UR") == 0 ? 1 : 0;
        if (mode != selected_channel_last_state->flags.mode) {
            selected_channel_last_state->flags.mode = mode;
            changed = true;
        }
        value = Value(mode);
    } else if (id == DATA_ID_MON_VALUE) {
        float mon_value;
        Unit unit;
        char *mode_str = selected_channel->getCvModeStr();
        if (strcmp(mode_str, "CC") == 0) {
            // CC -> volt
            mon_value = selected_channel->u.mon;
            unit = UNIT_VOLT;
        } else if (strcmp(mode_str, "CV") == 0) {
            // CV -> curr
            mon_value = selected_channel->i.mon;
            unit = UNIT_AMPER;
        } else {
            // UR ->
            if (selected_channel->u.mon < selected_channel->i.mon) {
                // min(volt, curr)
                mon_value = selected_channel->u.mon;
                unit = UNIT_VOLT;
            } else {
                // or curr if equal
                mon_value = selected_channel->i.mon;
                unit = UNIT_AMPER;
            }
        }
        value = Value(mon_value, unit);
        if (selected_channel_last_state->mon_value != value) {
            selected_channel_last_state->mon_value = value;
            changed = true;
        }
    } else if (id == DATA_ID_VOLT) {
        float u_set = selected_channel->u.set;
        if (selected_channel_last_state->u_set != u_set) {
            selected_channel_last_state->u_set = u_set;
            changed = true;
        }
        value = Value(u_set, UNIT_VOLT);
    } else if (id == DATA_ID_CURR) {
        float i_set = selected_channel->i.set;
        if (selected_channel_last_state->i_set != selected_channel->i.set) {
            selected_channel_last_state->i_set = selected_channel->i.set;
            changed = true;
        }
        value = Value(i_set, UNIT_AMPER);
    } else if (id == DATA_ID_OVP) {
        uint8_t ovp;
        if (!selected_channel->prot_conf.flags.u_state) ovp = 1;
        else if (!selected_channel->ovp.flags.tripped) ovp = 2;
        else ovp = 3;
        if (selected_channel_last_state->flags.ovp != ovp) {
            selected_channel_last_state->flags.ovp = ovp;
            changed = true;
        }
        value = Value(ovp);
    } else if (id == DATA_ID_OCP) {
        uint8_t ocp;
        if (!selected_channel->prot_conf.flags.i_state) ocp = 1;
        else if (!selected_channel->ocp.flags.tripped) ocp = 2;
        else ocp = 3;
        if (selected_channel_last_state->flags.ocp != ocp) {
            selected_channel_last_state->flags.ocp = ocp;
            changed = true;
        }
        value = Value(ocp);
    } else if (id == DATA_ID_OPP) {
        uint8_t opp;
        if (!selected_channel->prot_conf.flags.p_state) opp = 1;
        else if (!selected_channel->opp.flags.tripped) opp = 2;
        else opp = 3;
        if (selected_channel_last_state->flags.opp != opp) {
            selected_channel_last_state->flags.opp = opp;
            changed = true;
        }
        value = Value(opp);
    } else if (id == DATA_ID_OTP) {
        uint8_t otp;
        if (!temperature::prot_conf[temp_sensor::MAIN].state) otp = 1;
        else if (!temperature::isSensorTripped(temp_sensor::MAIN)) otp = 2;
        else otp = 3;
        if (selected_channel_last_state->flags.otp != otp) {
            selected_channel_last_state->flags.otp = otp;
            changed = true;
        }
        value = Value(otp);
    } else if (id == DATA_ID_DP) {
        uint8_t dp = selected_channel->flags.dp_on ? 1 : 2;
        if (selected_channel_last_state->flags.dp != dp) {
            selected_channel_last_state->flags.dp = dp;
            changed = true;
        }
        value = Value(dp);
    } else if (id == DATA_ID_ALERT_MESSAGE) {
        value = alert_message;
    } else if (id == DATA_ID_EDIT_VALUE) {
        Channel *selected_channel = &Channel::get(edit_data_cursor.selected_channel_index);
        if (edit_data_id == DATA_ID_VOLT)
            value = Value(selected_channel->u.set, UNIT_VOLT);
        else
            value = Value(selected_channel->i.set, UNIT_AMPER);

        if (value.getFloat() != last_edit_value) {
            last_edit_value = value.getFloat();
            changed = true;
        }
    } else if (id == DATA_ID_EDIT_UNIT) {
        Unit edit_unit = keypad::get_edit_unit();
        changed = edit_unit != last_edit_unit;
        last_edit_unit = edit_unit;
        if (edit_unit == UNIT_VOLT)
            value = Value::ConstStr("mV");
        else if (edit_unit == UNIT_MILLI_VOLT)
            value = Value::ConstStr("V");
        else if (edit_unit == UNIT_AMPER)
            value = Value::ConstStr("mA");
        else if (edit_unit == UNIT_MILLI_AMPER)
            value = Value::ConstStr("A");
    } else if (id == DATA_ID_EDIT_INFO) {
        Channel *selected_channel = &Channel::get(edit_data_cursor.selected_channel_index);

        if (edit_data_id == DATA_ID_VOLT) {
            sprintf_P(edit_info, PSTR("Set Ch%d voltage [%d-%d V]"), selected_channel->index,
                (int)selected_channel->U_MIN,
                (int)selected_channel->U_MAX);
        }
        else {
            sprintf_P(edit_info, PSTR("Set Ch%d current [%d-%d A]"), selected_channel->index,
                (int)selected_channel->I_MIN,
                (int)selected_channel->I_MAX);
        }

        if (strcmp(edit_info, last_edit_info) != 0) {
            strcpy(last_edit_info, edit_info);
            changed = true;
        }

        value = edit_info;
    }

    return value;
}

Value getMin(uint8_t id) {
    Channel *selected_channel = &Channel::get(cursor.selected_channel_index);

    Value value;
    if (id == DATA_ID_VOLT) {
        value = Value(selected_channel->U_MIN, UNIT_VOLT);
    } else if (id == DATA_ID_CURR) {
        value = Value(selected_channel->I_MIN, UNIT_AMPER);
    }
    return value;
}

Value getMax(uint8_t id) {
    Channel *selected_channel = &Channel::get(cursor.selected_channel_index);

    Value value;
    if (id == DATA_ID_VOLT) {
        value = Value(selected_channel->U_MAX, UNIT_VOLT);
    } else if (id == DATA_ID_CURR) {
        value = Value(selected_channel->I_MAX, UNIT_AMPER);
    }
    return value;
}

Unit getUnit(uint8_t id) {
    if (id == DATA_ID_VOLT) {
        return UNIT_VOLT;
    }
    else if (id == DATA_ID_CURR) {
        return UNIT_AMPER;
    }
    return UNIT_NONE;
}

void set(uint8_t id, Value value) {
    Channel *selected_channel = &Channel::get(cursor.selected_channel_index);

    if (id == DATA_ID_VOLT) {
        selected_channel->setVoltage(value.getFloat());
    } else if (id == DATA_ID_CURR) {
        selected_channel->setCurrent(value.getFloat());
    } else if (id == DATA_ID_ALERT_MESSAGE) {
        alert_message = value;
    }
}

void do_action(uint8_t id) {
    Channel *selected_channel = &Channel::get(cursor.selected_channel_index);

    if (id == ACTION_ID_TOGGLE_CHANNEL) {
        selected_channel->outputEnable(!selected_channel->isOutputEnabled());
    }
}

}
}
}
} // namespace eez::psu::ui::data