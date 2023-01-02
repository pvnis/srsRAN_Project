/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsgnb/radio/radio_configuration.h"

namespace srsgnb {

/// Radio configuration validator for ZMQ based radios.
class radio_config_zmq_config_validator : public radio_configuration::validator
{
public:
  // See interface for documentation.
  bool is_configuration_valid(const radio_configuration::radio& config) const override;
};

} // namespace srsgnb