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

#include "srsgnb/phy/upper/uplink_processor.h"

namespace srsgnb {

class uplink_processor_spy : public uplink_processor
{
  bool has_proces_prach_method_called = false;

public:
  void process_prach(const prach_buffer& buffer, const prach_buffer_context& context) override
  {
    has_proces_prach_method_called = true;
  }

  bool has_process_prach_method_called() const { return has_proces_prach_method_called; }
};

} // namespace srsgnb
