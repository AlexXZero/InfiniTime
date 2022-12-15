#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Components {
    enum class Event {
      UnixTime,   // Data is amount of seconds from 1970 (31 bits) up to 2038 year.
      TimeDiff,   // ms elapsed from previous stored event. 24 bits (up to 4.6 hours between events)
      Simple,     // 2^24 events without data
      Event16,    // 2^8 16-bit events (1 extra byte - type, 2 bytes - data)
    };

    enum class SimpleEvent : uint32_t {
      ColdStart,
      SoftwareReset,
      WdtReset,
      BleConnect,
      BleDisconnect,
      ChargeStart,
      ChargeStop,
      DfuStart,
      DfuStop,
    };

    enum class Event16Event : uint8_t {
      VccData,      // millivolts? TBD: Would be RAW ADC units better? Probably not, as it might be possible to use ADC scaling for increasing accuracy
      AccData,      // TBD: 4 bits for a number of a sample + 12 bits for the sample data
      TempUc,       // 0.1 degC units (-3273.8 to 3276.7 degC)
      StepCounter,  // Daily step counter
    };
  }
}
