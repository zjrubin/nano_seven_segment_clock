
#pragma once

#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

extern RtcDS3231<TwoWire> Rtc;

void setup_rtc_ds3231();

inline RtcDateTime get_datetime()
{
    return Rtc.GetDateTime();
}
