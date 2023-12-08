#pragma once

#include "slice.h"

#define MAX_NOF_SLICES 16

namespace srsran {

/// Container that stores all scheduler UEs.
class slice_repository
{
  using slice_list = std::array<std::unique_ptr<slice>,MAX_NOF_SLICES>;

public:
  /// \brief Add new UE in the UE repository.
  void add_slice(std::unique_ptr<slice> slice);

  /// \brief Remove existing slice from the repository.
  void remove_slice(int index);

  size_t size() const { return slices.size(); }

  bool empty() const { return slices.empty(); }

  int plmn_id(int MCC, int MNC, int MNC_len) const { return MCC*10**(MNC_len) + MNC; }

  iterator       begin() { return slices.begin(); }
  iterator       end() { return slices.end(); }
  const_iterator begin() const { return slices.begin(); }
  const_iterator end() const { return slices.end(); }

private:
  srslog::basic_logger&         logger;

  slice_list slices;
};

} // namespace srsran
