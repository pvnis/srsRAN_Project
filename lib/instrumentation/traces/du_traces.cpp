/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsran/instrumentation/traces/du_traces.h"

srsran::file_event_tracer<srsran::L2_TRACE_ENABLED> srsran::l2_tracer;

boost::array<double, 10> probs = {0.10, 0.25, 0.50, 0.75, 0.90, 0.95, 0.98, 0.99, 0.995, 1.00};

srsran::accumulator_t srsran::rlc_queue_time_acc(ba::extended_p_square_probabilities = probs);
