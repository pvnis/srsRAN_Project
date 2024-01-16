/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
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

#pragma once

namespace srsran {

/// Describes the base interface for radio classes.
class radio_base
{
protected:
  /// Allow default constructor for child.
  radio_base() = default;

public:
  /// Provides default destructor.
  virtual ~radio_base() = default;

  /// Forbid copy constructor.
  radio_base(const radio_base& other) = delete;

  /// Forbid move constructor.
  radio_base(const radio_base&& other) = delete;

  /// Forbid copy assigment operator.
  radio_base& operator=(const radio_base&) = delete;

  /// Forbid move assigment operator.
  radio_base& operator=(radio_base&&) = delete;
};
} // namespace srsran
