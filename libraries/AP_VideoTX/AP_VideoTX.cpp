/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AP_VideoTX.h"

#if AP_VIDEOTX_ENABLED

#include <AP_RCTelemetry/AP_CRSF_Telem.h>
#include <GCS_MAVLink/GCS.h>

#include <AP_HAL/AP_HAL.h>

extern const AP_HAL::HAL& hal;

AP_VideoTX *AP_VideoTX::singleton;

const AP_Param::GroupInfo AP_VideoTX::var_info[] = {

    // @Param: ENABLE
    // @DisplayName: Is the Video Transmitter enabled or not
    // @Description: Toggles the Video Transmitter on and off
    // @Values: 0:Disable,1:Enable
    AP_GROUPINFO_FLAGS("ENABLE", 1, AP_VideoTX, _enabled, 0, AP_PARAM_FLAG_ENABLE),

    // @Param: POWER
    // @DisplayName: Video Transmitter Power Level
    // @Description: Video Transmitter Power Level. Different VTXs support different power levels, the power level chosen will be rounded down to the nearest supported power level
    // @Range: 1 1000
    AP_GROUPINFO("POWER",    2, AP_VideoTX, _power_mw, 0),

    // @Param: CHANNEL
    // @DisplayName: Video Transmitter Channel
    // @Description: Video Transmitter Channel
    // @User: Standard
    // @Range: 0 7
    AP_GROUPINFO("CHANNEL",  3, AP_VideoTX, _channel, 0),

    // @Param: BAND
    // @DisplayName: Video Transmitter Band
    // @Description: Video Transmitter Band
    // @User: Standard
    // @Values: 0:Band A,1:Band B,2:Band E,3:Airwave,4:RaceBand,5:Low RaceBand,6:1G3 Band A,7:1G3 Band B,8:Band X,9:3G3 Band A,10:3G3 Band B
    AP_GROUPINFO("BAND",  4, AP_VideoTX, _band, 0),

    // @Param: FREQ
    // @DisplayName: Video Transmitter Frequency
    // @Description: Video Transmitter Frequency. The frequency is derived from the setting of BAND and CHANNEL
    // @User: Standard
    // @ReadOnly: True
    // @Range: 1000 6000
    AP_GROUPINFO("FREQ",  5, AP_VideoTX, _frequency_mhz, 0),

    // @Param: OPTIONS
    // @DisplayName: Video Transmitter Options
    // @Description: Video Transmitter Options. Pitmode puts the VTX in a low power state. Unlocked enables certain restricted frequencies and power levels. Do not enable the Unlocked option unless you have appropriate permissions in your jurisdiction to transmit at high power levels. One stop-bit may be required for VTXs that erroneously mimic iNav behaviour.
    // @User: Advanced
    // @Bitmask: 0:Pitmode,1:Pitmode until armed,2:Pitmode when disarmed,3:Unlocked,4:Add leading zero byte to requests,5:Use 1 stop-bit in SmartAudio,6:Ignore CRC in SmartAudio,7:Ignore status updates in CRSF and blindly set VTX options,8:Enable custom power
    AP_GROUPINFO("OPTIONS",  6, AP_VideoTX, _options, 0),

    // @Param: MAX_POWER
    // @DisplayName: Video Transmitter Max Power Level
    // @Description: Video Transmitter Maximum Power Level. Different VTXs support different power levels, this prevents the power aux switch from requesting too high a power level. The switch supports 6 power levels and the selected power will be a subdivision between 0 and this setting.
    // @Range: 25 1000
    AP_GROUPINFO("MAX_POWER", 7, AP_VideoTX, _max_power_mw, 800),

    // @Param: PLEVEL_CNT
    // @DisplayName: Video Transmitter power level count
    // @Description: Count for video Power Levels. If this parameter is greater than zero, only values ​​from VTX_PLEVELx are transferred to VTX.
    // @Range: 0 10
    AP_GROUPINFO("PLEVEL_CNT", 8, AP_VideoTX, _plevel_cnt, 0),

    // @Param: PLEVEL1
    // @DisplayName: Video Transmitter power level 1
    // @Description: Level 1 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL1", 9, AP_VideoTX, _plevel[0], 0),

    // @Param: PLEVEL2
    // @DisplayName: Video Transmitter power level 2
    // @Description: Level 2 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL2", 10, AP_VideoTX, _plevel[1], 0),

    // @Param: PLEVEL3
    // @DisplayName: Video Transmitter power level 3
    // @Description: Level 3 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL3", 11, AP_VideoTX, _plevel[2], 0),

    // @Param: PLEVEL4
    // @DisplayName: Video Transmitter power level 4
    // @Description: Level 4 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL4", 12, AP_VideoTX, _plevel[3], 0),

    // @Param: PLEVEL5
    // @DisplayName: Video Transmitter power level 5
    // @Description: Level 5 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL5", 13, AP_VideoTX, _plevel[4], 0),

    // @Param: PLEVEL6
    // @DisplayName: Video Transmitter power level 6
    // @Description: Level 6 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL6", 14, AP_VideoTX, _plevel[5], 0),

    // @Param: PLEVEL7
    // @DisplayName: Video Transmitter power level 7
    // @Description: Level 7 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL7", 15, AP_VideoTX, _plevel[6], 0),

    // @Param: PLEVEL8
    // @DisplayName: Video Transmitter power level 8
    // @Description: Level 8 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL8", 16, AP_VideoTX, _plevel[7], 0),

    // @Param: PLEVEL9
    // @DisplayName: Video Transmitter power level 9
    // @Description: Level 9 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL9", 17, AP_VideoTX, _plevel[8], 0),

    // @Param: PLEVEL10
    // @DisplayName: Video Transmitter power level 10
    // @Description: Level 10 of power VTX. Values from VTX_PLEVELx used if VTX_PLEVEL_CNT greater than zero. Otherwise, the default table is used.
    // @Range: 0 5000
    AP_GROUPINFO("PLEVEL10", 18, AP_VideoTX, _plevel[9], 0),

    // @Param: POWER_CTRL1
    // @DisplayName: No of VTX_PLEVELx in RC position 1
    // @Description: If RCx_OPTION is 94:VTX POWER value VTX_PLEVELx (x is the value VTX_POWER_CTRL1), used for RC channel position 1. 0 - disable this position. 
    // @Range: 0 6
    AP_GROUPINFO("POWER_CTRL1", 19, AP_VideoTX, _pcontrol[0], 0),

    // @Param: POWER_CTRL2
    // @DisplayName: No of VTX_PLEVELx in RC position 2
    // @Description: If RCx_OPTION is 94:VTX POWER value VTX_PLEVELx (x is the value VTX_POWER_CTRL2), used for RC channel position 2. 0 - disable this position. 
    // @Range: 0 6
    AP_GROUPINFO("POWER_CTRL2", 20, AP_VideoTX, _pcontrol[1], 0),

    // @Param: POWER_CTRL3
    // @DisplayName: No of VTX_PLEVELx in RC position 3
    // @Description: If RCx_OPTION is 94:VTX POWER value VTX_PLEVELx (x is the value VTX_POWER_CTRL3), used for RC channel position 3. 0 - disable this position. 
    // @Range: 0 6
    AP_GROUPINFO("POWER_CTRL3", 21, AP_VideoTX, _pcontrol[2], 0),

    // @Param: POWER_CTRL4
    // @DisplayName: No of VTX_PLEVELx in RC position 4
    // @Description: If RCx_OPTION is 94:VTX POWER value VTX_PLEVELx (x is the value VTX_POWER_CTRL4), used for RC channel position 4. 0 - disable this position. 
    // @Range: 0 6
    AP_GROUPINFO("POWER_CTRL4", 22, AP_VideoTX, _pcontrol[3], 0),

    // @Param: POWER_CTRL5
    // @DisplayName: No of VTX_PLEVELx in RC position 5
    // @Description: If RCx_OPTION is 94:VTX POWER value VTX_PLEVELx (x is the value VTX_POWER_CTRL5), used for RC channel position 5. 0 - disable this position. 
    // @Range: 0 6
    AP_GROUPINFO("POWER_CTRL5", 23, AP_VideoTX, _pcontrol[4], 0),

    // @Param: POWER_CTRL6
    // @DisplayName: No of VTX_PLEVELx in RC position 6
    // @Description: If RCx_OPTION is 94:VTX POWER value VTX_PLEVELx (x is the value VTX_POWER_CTRL6), used for RC channel position 6. 0 - disable this position.
    // @Range: 0 6
    AP_GROUPINFO("POWER_CTRL6", 24, AP_VideoTX, _pcontrol[5], 0),

    // @Param: FREQ_CTRL1
    // @DisplayName: Band and frequency in RC position 1
    // @Description: If RCx_OPTION is 178:VTX Frequency value VTX_FREQ_CTRL1 set band and channel of VTX. Value 0 - set band 0 and channel 0, value 1 - band 0 channel 1, value 15 - band 1 channel 5. -1 - no change frequnce if RCx in this position.
    // @Range: -1 999
    AP_GROUPINFO("FREQ_CTRL1", 25, AP_VideoTX, _fcontrol[0], -1),

    // @Param: FREQ_CTRL2
    // @DisplayName: Band and frequency in RC position 2
    // @Description: If RCx_OPTION is 178:VTX Frequency value VTX_FREQ_CTRL2 set band and channel of VTX. Value 0 - set band 0 and channel 0, value 1 - band 0 channel 1, value 15 - band 1 channel 5. -1 - no change frequnce if RCx in this position.
    // @Range: -1 999
    AP_GROUPINFO("FREQ_CTRL2", 26, AP_VideoTX, _fcontrol[1], -1),

    // @Param: FREQ_CTRL3
    // @DisplayName: Band and frequency in RC position 3
    // @Description: If RCx_OPTION is 178:VTX Frequency value VTX_FREQ_CTRL3 set band and channel of VTX. Value 0 - set band 0 and channel 0, value 1 - band 0 channel 1, value 15 - band 1 channel 5. -1 - no change frequnce if RCx in this position.
    // @Range: -1 999
    AP_GROUPINFO("FREQ_CTRL3", 27, AP_VideoTX, _fcontrol[2], -1),

    // @Param: FREQ_CTRL4
    // @DisplayName: Band and frequency in RC position 4
    // @Description: If RCx_OPTION is 178:VTX Frequency value VTX_RC_CTRL4 set band and channel of VTX. Value 0 - set band 0 and channel 0, value 1 - band 0 channel 1, value 15 - band 1 channel 5. -1 - no change frequnce if RCx in this position.
    // @Range: -1 999
    AP_GROUPINFO("FREQ_CTRL4", 28, AP_VideoTX, _fcontrol[3], -1),

    // @Param: FREQ_CTRL5
    // @DisplayName: Band and frequency in RC position 5
    // @Description: If RCx_OPTION is 178:VTX Frequency value VTX_FREQ_CTRL5 set band and channel of VTX. Value 0 - set band 0 and channel 0, value 1 - band 0 channel 1, value 15 - band 1 channel 5. -1 - no change frequnce if RCx in this position.
    // @Range: -1 999
    AP_GROUPINFO("FREQ_CTRL5", 29, AP_VideoTX, _fcontrol[4], -1),

    // @Param: FREQ_CTRL6
    // @DisplayName: Band and frequency in RCx position 6
    // @Description: If RCx_OPTION is 178:VTX Frequency, value VTX_FREQ_CTRL6 set band and channel of VTX. Value 0 - set band 0 and channel 0, value 1 - band 0 channel 1, value 15 - band 1 channel 5. -1 - no change frequnce if RCx in this position.
    // @Range: -1 999
    AP_GROUPINFO("FREQ_CTRL6", 30, AP_VideoTX, _fcontrol[5], -1),

    AP_GROUPEND
};

// #define VTX_DEBUG
#ifdef VTX_DEBUG
# define debug(fmt, args...)	hal.console->printf("VTX: " fmt "\n", ##args)
#else
# define debug(fmt, args...)	do {} while(0)
#endif
# define debug2(fmt, args...)	hal.console->printf("VTX: " fmt "\n", ##args)

extern const AP_HAL::HAL& hal;

const char * AP_VideoTX::band_names[] = {"A","B","E","F","R","L","1G3_A","1G3_B","X","3G3_A","3G3_B"};

const uint16_t AP_VideoTX::VIDEO_CHANNELS[AP_VideoTX::MAX_BANDS][VTX_MAX_CHANNELS] =
{
    { 5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725}, /* Band A */
    { 5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866}, /* Band B */
    { 5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945}, /* Band E */
    { 5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880}, /* Airwave */
    { 5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917}, /* Race */
    { 5621, 5584, 5547, 5510, 5473, 5436, 5399, 5362}, /* LO Race */
    { 1080, 1120, 1160, 1200, 1240, 1280, 1320, 1360}, /* Band 1G3_A */
    { 1080, 1120, 1160, 1200, 1258, 1280, 1320, 1360}, /* Band 1G3_B */
    { 4990, 5020, 5050, 5080, 5110, 5140, 5170, 5200}, /* Band X */
    { 3330, 3350, 3370, 3390, 3410, 3430, 3450, 3470}, /* Band 3G3_A */
    { 3170, 3190, 3210, 3230, 3250, 3270, 3290, 3310}  /* Band 3G3_B */
};

// mapping of power level to milliwatt to dbm
// valid power levels from SmartAudio spec, the adjacent levels might be the actual values
// so these are marked as level + 0x10 and will be switched if a dbm message proves it
AP_VideoTX::PowerLevel AP_VideoTX::_power_levels[VTX_MAX_POWER_LEVELS] = {
    // level, mw, dbm, dac
    { 0xFF,  0,    0, 0    }, // only in SA 2.1
    { 0,    25,   14, 7    },
    { 0x11, 100,  20, 0xFF }, // only in SA 2.1
    { 1,    200,  23, 16   },
    { 0x12, 400,  26, 0xFF }, // only in SA 2.1
    { 2,    500,  27, 25   },
    { 0x12, 600,  28, 0xFF }, // Tramp lies above power levels and always returns 25/100/200/400/600
    { 3,    800,  29, 40   },
    { 0x13, 1000, 30, 0xFF }, // only in SA 2.1
    { 0xFF, 0,    0,  0XFF, PowerActive::Inactive }  // slot reserved for a custom power level
};

AP_VideoTX::PowerLevel AP_VideoTX::_plevels[VTX_MAX_PLEVELS];

AP_VideoTX::AP_VideoTX()
{
    if (singleton) {
        AP_HAL::panic("Too many VTXs");
        return;
    }
    singleton = this;

    AP_Param::setup_object_defaults(this, var_info);
}

AP_VideoTX::~AP_VideoTX(void)
{
    singleton = nullptr;
}

bool AP_VideoTX::init(void)
{
    uint8_t pmax, pcnt;
    uint16_t plev;

    PowerLevel* ptable;
    if (_initialized) {
        return false;
    }
    for (uint8_t i=0; i< _plevel_cnt; i++) {
        plev=_plevel[i];
        _plevels[i] = {0xFF, plev,    0,  0XFF};
    }
    _plevels[VTX_MAX_PLEVELS-1] = {0xFF, 0,    0,  0XFF, PowerActive::Inactive};

    // PARAMETER_CONVERSION - Added: Sept-2022
    _options.convert_parameter_width(AP_PARAM_INT16);
    if (_plevel_cnt == 0) {
        pmax=VTX_MAX_POWER_LEVELS;
        pcnt=VTX_MAX_POWER_LEVELS-1;
        ptable = _power_levels;
    } else {
        pmax=VTX_MAX_PLEVELS;
        pcnt=_plevel_cnt;
        ptable = _plevels;
    }

    if (_options & (uint16_t)VideoOptions::VTX_CUSTOM_POWER) {
        _current_power = pmax-1;
    } else {
        _current_power=0;
        // find the index into the power table
        for (uint8_t i = 0; i < pcnt-1; i++) {
            if (ptable[i].mw !=0) {
                if (_power_mw <= ptable[i].mw) {
                    if (_power_mw != ptable[i].mw) {
                        if (i > 0) {
                            _current_power = i - 1;
                        }
                        _power_mw.set_and_save(get_power_mw());
                    } else {
                        _current_power = i;
                    }
                    break;
                }
            }
        }
    }
    _current_band = _band;
    _current_channel = _channel;
    _current_frequency = _frequency_mhz;
    _current_options = _options;
    _current_enabled = _enabled;
    _initialized = true;

    return true;
}

bool AP_VideoTX::get_band_and_channel(uint16_t freq, VideoBand& band, uint8_t& channel)
{
    for (uint8_t i = 0; i < AP_VideoTX::MAX_BANDS; i++) {
        for (uint8_t j = 0; j < VTX_MAX_CHANNELS; j++) {
            if (VIDEO_CHANNELS[i][j] == freq) {
                band = VideoBand(i);
                channel = j;
                return true;
            }
        }
    }
    return false;
}

// set the current power
void AP_VideoTX::set_configured_power_mw(uint16_t power)
{
    _power_mw.set_and_save_ifchanged(power);
}

uint8_t AP_VideoTX::find_current_power() const
{
    uint8_t pmax, pcnt;
    PowerLevel* ptable;
    if (_plevel_cnt == 0) {
        pmax=VTX_MAX_POWER_LEVELS;
        pcnt=VTX_MAX_POWER_LEVELS-1;
        ptable = _power_levels;
    } else {
        pmax=VTX_MAX_PLEVELS;
        pcnt=_plevel_cnt;
        ptable = _plevels;
    }

    if (_options & (uint16_t)VideoOptions::VTX_CUSTOM_POWER) {
        return pmax-1;
    } else {
        for (uint8_t i = 0; i < pcnt-1; i++) {
            if  ((ptable[i].mw != 0) and (_power_mw == ptable[i].mw)) {
                return i;
            }
        }
    }
    return 0;
}

// set the power in dbm, rounding appropriately
void AP_VideoTX::set_power_dbm(uint8_t power, PowerActive active)
{
    if (power == _power_levels[_current_power].dbm
        && _power_levels[_current_power].active == active) {
        return;
    }

    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].dbm) {
            _current_power = i;
            _power_levels[i].active = active;
            debug("learned power %ddbm", power);
            // now unlearn the "other" power level since we have no other way of guessing
            // the supported levels
            if ((_power_levels[i].level & 0xF0) == 0x10) {
                _power_levels[i].level = _power_levels[i].level & 0xF;
            }
            if (i > 0 && _power_levels[i-1].level == _power_levels[i].level) {
                debug("invalidated power %dwm, level %d is now %dmw", _power_levels[i-1].mw, _power_levels[i].level, _power_levels[i].mw);
                _power_levels[i-1].level = 0xFF;
                _power_levels[i-1].active = PowerActive::Inactive;
            } else if (i < VTX_MAX_POWER_LEVELS-1 && _power_levels[i+1].level == _power_levels[i].level) {
                debug("invalidated power %dwm, level %d is now %dmw", _power_levels[i+1].mw, _power_levels[i].level, _power_levels[i].mw);
                _power_levels[i+1].level = 0xFF;
                _power_levels[i+1].active = PowerActive::Inactive;
            }
            return;
        }
    }
    // learn the non-standard power
    _current_power = update_power_dbm(power, active);
}

// add an active power setting in dbm
uint8_t AP_VideoTX::update_power_dbm(uint8_t power, PowerActive active)
{
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].dbm) {
            if (_power_levels[i].active != active) {
                _power_levels[i].active = active;
                debug("%s power %ddbm", active == PowerActive::Active ? "learned" : "invalidated", power);
            }
            return i;
        }
    }
    // handed a non-standard value, use the last slot
    _power_levels[VTX_MAX_POWER_LEVELS-1].dbm = power;
    _power_levels[VTX_MAX_POWER_LEVELS-1].level = 255;
    _power_levels[VTX_MAX_POWER_LEVELS-1].dac = 255;
    _power_levels[VTX_MAX_POWER_LEVELS-1].mw = uint16_t(roundf(powf(10, power / 10.0f)));
    _power_levels[VTX_MAX_POWER_LEVELS-1].active = active;
    debug("non-standard power %ddbm -> %dmw", power, _power_levels[VTX_MAX_POWER_LEVELS-1].mw);
    return VTX_MAX_POWER_LEVELS-1;
}

// add all active power setting in dbm
void AP_VideoTX::update_all_power_dbm(uint8_t nlevels, const uint8_t power[])
{
    for (uint8_t i = 0; i < nlevels; i++) {
        update_power_dbm(power[i], PowerActive::Active);
    }
    // invalidate the remaining ones
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (_power_levels[i].active == PowerActive::Unknown) {
            _power_levels[i].active = PowerActive::Inactive;
        }
    }
}

uint16_t AP_VideoTX::get_power_mw() const { 
    if (_plevel_cnt == 0) 
        return _power_levels[_current_power].mw; 
    else
        return _plevels[_current_power].mw; 

}

// set the power in mw
void AP_VideoTX::set_power_mw(uint16_t power)
{
    uint8_t pmax, pcnt;
    PowerLevel* ptable;
    if (_plevel_cnt == 0) {
        pmax=VTX_MAX_POWER_LEVELS;
        pcnt=VTX_MAX_POWER_LEVELS-1;
        ptable = _power_levels;
    } else {
        pmax=VTX_MAX_PLEVELS;
        pcnt=_plevel_cnt;
        ptable = _plevels;
    }

    if (_options & (uint16_t)VideoOptions::VTX_CUSTOM_POWER) {
        ptable[pmax-1].mw = power;
        ptable[pmax-1].active =  PowerActive::Active;
        _current_power = pmax-1;
        return;
    }
    // if not custom power set _custom_power  from table
    _current_power =0;
    for (uint8_t i = 0; i < pcnt-1; i++) {
        if (ptable[i].mw !=0) {
            if (power <= ptable[i].mw) {
                if (power != ptable[i].mw) {
                    if (i > 0) {
                        _current_power = i - 1;
                    }
                } else {
                    _current_power =i;
                }
                break;
            }
        }
    }
    // oled find _current_power
/*
    for (uint8_t i = 0; i < pcnt; i++) {
        if (power == ptable[i].mw) {
            _current_power = i;
            break;
        }
    }
*/
}

// set the power "level"
void AP_VideoTX::set_power_level(uint8_t level, PowerActive active)
{
    if (level == _power_levels[_current_power].level
        && _power_levels[_current_power].active == active) {
        return;
    }

    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (level == _power_levels[i].level) {
            _current_power = i;
            _power_levels[i].active = active;
            debug("learned power level %d: %dmw", level, get_power_mw());
            break;
        }
    }
}

// set the power dac
void AP_VideoTX::set_power_dac(uint16_t power, PowerActive active)
{
    if (power == _power_levels[_current_power].dac
        && _power_levels[_current_power].active == active) {
        return;
    }

    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].dac) {
            _current_power = i;
            _power_levels[i].active = active;
            debug("learned power %dmw", get_power_mw());
        }
    }
}

// set the current channel
void AP_VideoTX::set_enabled(bool enabled)
{
    _current_enabled = enabled;
    if (!_enabled.configured()) {
        _enabled.set_and_save(enabled);
    }
}

void AP_VideoTX::set_power_is_current()
{
    set_power_dbm(get_configured_power_dbm());
}
/*
void AP_VideoTX::set_band(uint8_t band) { 
    debug2("set_band: _current_band: %d, new band %d, _band %d",  _current_band, band, _band.get());
    _current_band = band;
}

void AP_VideoTX::set_configured_band(uint8_t band) { 
    debug2("set_configured_band: Current band: %d, new band %d, _current_band: %d", _band.get(),band, _current_band);
    _band.set_and_save_ifchanged(band);
    _band.set_and_save(band);
    uint16_t freq= get_frequency_mhz(band,get_configured_channel());
    debug2("New frequency: %d", freq);

    set_configured_frequency_mhz(freq);
}
*/
void AP_VideoTX::set_freq_is_current()
{
    _current_frequency = _frequency_mhz;
    _current_band = _band;
    _current_channel = _channel;
}


// periodic update
void AP_VideoTX::update(void)
{
    PowerLevel* ptable;

    if (!_enabled) {
        return;
    }

    if (_plevel_cnt == 0) {
        ptable = _power_levels;
    } else {
        ptable = _plevels;
    }

    // manipulate pitmode if pitmode-on-disarm or power-on-arm is set
    if (has_option(VideoOptions::VTX_PITMODE_ON_DISARM) || has_option(VideoOptions::VTX_PITMODE_UNTIL_ARM)) {
        if (hal.util->get_soft_armed() && has_option(VideoOptions::VTX_PITMODE)) {
            _options.set(_options & ~uint8_t(VideoOptions::VTX_PITMODE));
        } else if (!hal.util->get_soft_armed() && !has_option(VideoOptions::VTX_PITMODE)
            && has_option(VideoOptions::VTX_PITMODE_ON_DISARM)) {
            _options.set(_options | uint8_t(VideoOptions::VTX_PITMODE));
        }
    }
    // check that the requested power is actually allowed
    // reset if not
    if (_power_mw != get_power_mw()) {
        if (ptable[find_current_power()].active == PowerActive::Inactive) {
            // reset to something we know works
            debug("power reset to %dmw from %dmw", get_power_mw(), _power_mw.get());
            _power_mw.set_and_save(get_power_mw());
        }
    }
}

bool AP_VideoTX::update_options() const
{
    if (!_defaults_set) {
        return false;
    }
    // check pitmode
    if ((_options & uint8_t(VideoOptions::VTX_PITMODE))
        != (_current_options & uint8_t(VideoOptions::VTX_PITMODE))) {
        return true;
    }

#if HAL_CRSF_TELEM_ENABLED
    // using CRSF so unlock is not an option
    if (AP::crsf_telem() != nullptr) {
        return false;
    }
#endif
    // check unlock only
    if ((_options & uint8_t(VideoOptions::VTX_UNLOCKED)) != 0
        && (_current_options & uint8_t(VideoOptions::VTX_UNLOCKED)) == 0) {
        return true;
    }

    // ignore everything else
    return false;
}

bool AP_VideoTX::update_power() const {
    uint8_t pcnt;
    PowerLevel* ptable;
    // if power not changed - not update
    if (!_defaults_set || _power_mw == get_power_mw() || get_pitmode()) {
        return false;
    }

    // if power changed and enable custom power - can update
    if (_options & uint16_t(VideoOptions::VTX_CUSTOM_POWER)) {
        return true;
    }
    // check that the requested power is actually allowed
    if (_plevel_cnt == 0) {
        pcnt=VTX_MAX_POWER_LEVELS-1;
        ptable = _power_levels;
    } else {
        pcnt=_plevel_cnt;
        ptable = _plevels;
    }

    for (uint8_t i = 0; i < pcnt; i++) {

        if (_power_mw == ptable[i].mw
            && ptable[i].active != PowerActive::Inactive) {
            return true;
        }
    }
    // asked for something unsupported - only SA2.1 allows this and will have already provided a list
    return false;
}

bool AP_VideoTX::have_params_changed() const
{
    // GCS_SEND_TEXT(MAV_SEVERITY_INFO,"update");
    return _enabled
        && (update_power()
        || update_band()
        || update_channel()
        || update_frequency()
        || update_options());
}

// update the configured frequency to match the channel and band
void AP_VideoTX::update_configured_frequency()
{
    // _frequency_mhz.set_and_save(freq);
    _frequency_mhz.set_and_save(get_frequency_mhz(_band, _channel));
}

// update the configured channel and band to match the frequency
void AP_VideoTX::update_configured_channel_and_band()
{
    VideoBand band;
    uint8_t channel;

    if (get_band_and_channel(_frequency_mhz, band, channel)) {
        _band.set_and_save(band);
        _channel.set_and_save(channel);
    } else {
        update_configured_frequency();
    }
}

// set the current configured values if not currently set in storage
// this is necessary so that the current settings can be seen
bool AP_VideoTX::set_defaults()
{
    if (_defaults_set) {
        return false;
    }

    // check that our current view of frequency matches band/channel
    // if not then force one to be correct
    uint16_t calced_freq = get_frequency_mhz(_current_band, _current_channel);
    if (_current_frequency != calced_freq) {
        if (_current_frequency > 0) {
            VideoBand band;
            uint8_t channel;
            if (get_band_and_channel(_current_frequency, band, channel)) {
                _current_band = band;
                _current_channel = channel;
            } else {
                _current_frequency = calced_freq;
            }
        } else {
            _current_frequency = calced_freq;
        }
    }

    if (!_options.configured()) {
        _options.set_and_save(_current_options);
    }
    if (!_channel.configured()) {
        _channel.set_and_save(_current_channel);
    }
    if (!_band.configured()) {
        _band.set_and_save(_current_band);
    }
    if (!_power_mw.configured()) {
        _power_mw.set_and_save(get_power_mw());
    }
    if (!_frequency_mhz.configured()) {
        _frequency_mhz.set_and_save(_current_frequency);
    }
    if (!_plevel_cnt.configured()) {
        _plevel_cnt.set_and_save(_current_frequency);
    }

    // Now check that the user didn't screw up by selecting incompatible options
    if (_frequency_mhz != get_frequency_mhz(_band, _channel)) {
        if (_frequency_mhz > 0) {
            update_configured_channel_and_band();
        } else {
            update_configured_frequency();
        }
    }

    _defaults_set = true;

    announce_vtx_settings();

    return true;
}

void AP_VideoTX::announce_vtx_settings() const
{
    // Output a friendly message so the user knows the VTX has been detected
    GCS_SEND_TEXT(MAV_SEVERITY_INFO, "VTX: %s%d %dMHz, PWR: %dmW",
        band_names[_band.get()], _channel.get() + 1, _frequency_mhz.get(),
        has_option(VideoOptions::VTX_PITMODE) ? 0 : _power_mw.get());
}

// change the video power based on switch input
// 6-pos range is in the middle of the available range
void AP_VideoTX::change_power(int8_t position)
{
    uint16_t power = 0;
    uint8_t num_active_levels = 0;

    if (!_enabled || position < 0 || position > 5) {
        return;
    }
    if (_plevel_cnt == 0) {
    // first find out how many possible levels there are
        for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
            if (_power_levels[i].active != PowerActive::Inactive && _power_levels[i].mw <= _max_power_mw) {
                num_active_levels++;
            }
        }
        // iterate through to find the level
        uint16_t level = constrain_int16(roundf((num_active_levels * (position + 1)/ 6.0f) - 1), 0, num_active_levels - 1);
        debug("looking for pos %d power level %d from %d", position, level, num_active_levels);
        // uint16_t power = 0;
        for (uint8_t i = 0, j = 0; i < num_active_levels; i++, j++) {
            while (j < VTX_MAX_POWER_LEVELS-1 && _power_levels[j].active == PowerActive::Inactive) {
                j++;
            }
            if (i == level) {
                power = _power_levels[j].mw;
                debug("selected power %dmw", power);
                break;
            }
        }
    } else {
        uint8_t current_control=_pcontrol[position];
        if (current_control !=0) {
            power=_plevels[current_control-1].mw;
        }

    }
    if (power == 0) {
        if (!hal.util->get_soft_armed()) {    // don't allow pitmode to be entered if already armed
            set_configured_options(get_configured_options() | uint8_t(VideoOptions::VTX_PITMODE));
        }
    } else {
        if (has_option(VideoOptions::VTX_PITMODE)) {
            set_configured_options(get_configured_options() & ~uint8_t(VideoOptions::VTX_PITMODE));
        }
        set_configured_power_mw(power);
    }
}

void AP_VideoTX::change_freq(int8_t position)
{
    uint16_t freq = 0;
    uint8_t band = 0;
    uint8_t channel = 0;


    if (!_enabled || position < 0 || position > 5) {
        return;
    }
    int8_t current_control=_fcontrol[position];
    if (current_control !=-1) {
        band=current_control / 10;
        channel=current_control % 10;
        freq=VIDEO_CHANNELS[band][channel];
    }

    if (freq != 0) {
        set_configured_band(band);
        set_configured_channel(channel);
    }
}

namespace AP {
    AP_VideoTX& vtx() {
        return *AP_VideoTX::get_singleton();
    }
};

#endif
