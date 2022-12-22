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

#include "../converters/rlc_config_helpers.h"
#include "srsgnb/mac/mac_cell_group_config.h"
#include "srsgnb/ran/du_types.h"
#include "srsgnb/ran/lcid.h"
#include "srsgnb/ran/physical_cell_group.h"
#include "srsgnb/rlc/rlc_config.h"
#include "srsgnb/scheduler/config/serving_cell_config.h"

namespace srsgnb {
namespace srs_du {

/// Serving cell specific MAC and PHY parameters for a SpCell.
struct spcell_config {
  du_cell_index_t     cell_index;
  serv_cell_index_t   serv_cell_idx;
  serving_cell_config spcell_cfg_ded;
};

/// \brief Secondary serving Cell configured for a UE.
struct scell_config {
  du_cell_index_t   cell_index;
  serv_cell_index_t serv_cell_index;
};

/// This struct stores the accumulated CellGroupConfig.
struct cell_group_config {
  std::vector<rlc_bearer_config> rlc_bearers;
  spcell_config                  spcell_cfg;
  mac_cell_group_config          mcg_cfg;
  physical_cell_group_config     pcg_cfg;
  slotted_vector<scell_config>   scells;
};

} // namespace srs_du
} // namespace srsgnb