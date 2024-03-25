//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

namespace eda::gate::optimizer {

/**
* \brief Interface for const iterator.
* Const iterator is used for read-only iteration over some collection.
* Can be used for lazy iteration implementation.
* \author <a href="mailto:mrpepelulka@gmail.com">Rustamkhan Ramaldanov</a>
*/
template<typename T>
class ConstIterator {
public:
  virtual ~ConstIterator() = default;

  virtual bool isEnd() const = 0;
  virtual bool next() = 0;
  virtual T get() const = 0;
  virtual size_t size() const = 0;
  virtual operator bool() const = 0;
};

} // namespace eda::gate::optimizer
