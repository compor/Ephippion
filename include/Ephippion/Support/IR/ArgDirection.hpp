//
//
//

#pragma once

#include "Ephippion/Config.hpp"

namespace ephippion {

enum ArgDirection : unsigned {
  AD_Inbound = 1,
  AD_Outbound = 2,
  AD_Both = AD_Inbound | AD_Outbound,
};

} // namespace ephippion

