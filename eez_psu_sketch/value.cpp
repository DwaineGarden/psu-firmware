/*
* EEZ PSU Firmware
* Copyright (C) 2017-present, Envox d.o.o.
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
#include "channel.h"
#include "channel_dispatcher.h"
#include "value.h"

namespace eez {
namespace psu {

ValueTypeTraits g_valueTypeTraits[] = {
    { "",     2                                         , powf(10.0f, 2.0f)                                               },
    { "V",    VOLTAGE_NUM_SIGNIFICANT_DECIMAL_DIGITS    , powf(10.0f, (float)VOLTAGE_NUM_SIGNIFICANT_DECIMAL_DIGITS)      },
    { "mV",   VOLTAGE_NUM_SIGNIFICANT_DECIMAL_DIGITS-3  , powf(10.0f, (float)(VOLTAGE_NUM_SIGNIFICANT_DECIMAL_DIGITS-3))  },
    { "A",    CURRENT_NUM_SIGNIFICANT_DECIMAL_DIGITS    , powf(10.0f, (float)CURRENT_NUM_SIGNIFICANT_DECIMAL_DIGITS)      },
    { "mA",   CURRENT_NUM_SIGNIFICANT_DECIMAL_DIGITS-3  , powf(10.0f, (float)(CURRENT_NUM_SIGNIFICANT_DECIMAL_DIGITS-3))  },
    { "W",    POWER_NUM_SIGNIFICANT_DECIMAL_DIGITS      , powf(10.0f, (float)POWER_NUM_SIGNIFICANT_DECIMAL_DIGITS)        },
    { "mW",   POWER_NUM_SIGNIFICANT_DECIMAL_DIGITS-3    , powf(10.0f, (float)(POWER_NUM_SIGNIFICANT_DECIMAL_DIGITS-3))    },
    { "s",    DURATION_NUM_SIGNIFICANT_DECIMAL_DIGITS   , powf(10.0f, (float)DURATION_NUM_SIGNIFICANT_DECIMAL_DIGITS)     },
    { "ms",   DURATION_NUM_SIGNIFICANT_DECIMAL_DIGITS-3 , powf(10.0f, (float)(DURATION_NUM_SIGNIFICANT_DECIMAL_DIGITS-3)) },
    { "oC",   TEMP_NUM_SIGNIFICANT_DECIMAL_DIGITS       , powf(10.0f, (float)TEMP_NUM_SIGNIFICANT_DECIMAL_DIGITS)         },
    { "rpm",  RPM_NUM_SIGNIFICANT_DECIMAL_DIGITS        , powf(10.0f, (float)RPM_NUM_SIGNIFICANT_DECIMAL_DIGITS)          },
    { "ohm",  LOAD_NUM_SIGNIFICANT_DECIMAL_DIGITS       , powf(10.0f, (float)LOAD_NUM_SIGNIFICANT_DECIMAL_DIGITS)         },
    { "Kohm", LOAD_NUM_SIGNIFICANT_DECIMAL_DIGITS       , powf(10.0f, (float)LOAD_NUM_SIGNIFICANT_DECIMAL_DIGITS)         },
    { "Mohm", LOAD_NUM_SIGNIFICANT_DECIMAL_DIGITS       , powf(10.0f, (float)LOAD_NUM_SIGNIFICANT_DECIMAL_DIGITS)         },
    { "%",    PERCENT_NUM_SIGNIFICANT_DECIMAL_DIGITS    , powf(10.0f, (float)PERCENT_NUM_SIGNIFICANT_DECIMAL_DIGITS)      }
};

float g_precisions[] = {
    1.0f,
    10.0f,
    100.0f,
    1000.0f,
    10000.0f,
    100000.0f
};

float getPrecision(float value, ValueType valueType, int channelIndex) {
    int numSignificantDecimalDigits = getNumSignificantDecimalDigits(valueType);

    if (util::greater(value, 99.99f, getPrecisionFromNumSignificantDecimalDigits(2))) {
        if (numSignificantDecimalDigits > 1) {
            numSignificantDecimalDigits = 1;
        }
    } else if (util::greater(value, 9.999f, getPrecisionFromNumSignificantDecimalDigits(2))) {
        if (numSignificantDecimalDigits > 2) {
            numSignificantDecimalDigits = 2;
        }
    } else {
        if (valueType == VALUE_TYPE_FLOAT_AMPER) {
            if (channelIndex != -1 && channel_dispatcher::isCurrentLowRangeAllowed(Channel::get(channelIndex)) && util::lessOrEqual(value, 0.5, getPrecision(VALUE_TYPE_FLOAT_AMPER))) {
                ++numSignificantDecimalDigits;
            }
        }
    }

    return getPrecisionFromNumSignificantDecimalDigits(numSignificantDecimalDigits);
}

const char *getUnitStr(ValueType valueType) {
    unsigned int index = valueType - VALUE_TYPE_FLOAT;
    if (index >= 0 && index < sizeof(g_valueTypeTraits) / sizeof(ValueTypeTraits)) {
        return g_valueTypeTraits[index].unitStr; 
    } else {
        return "";
    }
}

}
} // namespace eez::psu