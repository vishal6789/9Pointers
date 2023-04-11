#ifndef _TABLE_H_
#define _TABLE_H_

#include <SinricProDevice.h>
#include <Capabilities/PercentageController.h>
#include <Capabilities/RangeController.h>

class Table 
: public SinricProDevice
, public PercentageController<Table>
, public RangeController<Table> {
  friend class PercentageController<Table>;
  friend class RangeController<Table>;
public:
  Table(const String &deviceId) : SinricProDevice(deviceId, "Table") {};
};

#endif
