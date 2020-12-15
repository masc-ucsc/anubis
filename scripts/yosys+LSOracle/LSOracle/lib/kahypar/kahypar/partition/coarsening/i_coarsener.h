/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/

#pragma once

#include <string>

#include "kahypar/definitions.h"
#include "kahypar/macros.h"

namespace kahypar {
class IRefiner;

class ICoarsener {
 public:
  ICoarsener(const ICoarsener&) = delete;
  ICoarsener(ICoarsener&&) = delete;
  ICoarsener& operator= (const ICoarsener&) = delete;
  ICoarsener& operator= (ICoarsener&&) = delete;

  void coarsen(const HypernodeID limit) {
    coarsenImpl(limit);
  }

  bool uncoarsen(IRefiner& refiner) {
    return uncoarsenImpl(refiner);
  }

  virtual ~ICoarsener() = default;

 protected:
  ICoarsener() = default;

 private:
  virtual void coarsenImpl(const HypernodeID limit) = 0;
  virtual bool uncoarsenImpl(IRefiner& refiner) = 0;
};
}  // namespace kahypar
