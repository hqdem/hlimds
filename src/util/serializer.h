//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <istream>
#include <map>
#include <ostream>
#include <vector>

namespace eda::util {

/**
 * \brief Interface for classes which serialize and deserialize objects.
 */
template <class T>
class Serializer {
public:
  virtual void serialize(std::ostream &out, const T &obj) = 0;
  virtual T deserialize(std::istream &in) = 0;
};

template <class T>
void pullFromStream(std::istream &in, T &dest) {
  in.read((char*)&dest, sizeof(T));
  if (in.fail()) {
    throw std::runtime_error("Deserializaion: Failed to pull data from stream");
  }
}

template<typename T>
void pushIntoStream(std::ostream &out, const T &value) {
  out.write((char*)&value, sizeof(T));
  if (out.fail()) {
    throw std::runtime_error("Serialization: Failed to push data into stream");
  }
}

/**
 * \brief Most simple serializer.
 * It simply puts the data into the stream the same way it is stored in memory.
 */
template <class T>
class NaiveSerializer : public Serializer<T> {
public:
  void serialize(std::ostream &out, const T &value) override {
    pushIntoStream(out, value);
  }

  T deserialize(std::istream &in) override {
    T result;
    pullFromStream(in, result);
    return result;
  }
};

/**
 * \brief Serializer template class for std::map<T1, T2>.
 * Classes S1 and S2 are serializer classes for T1 and T2 respectively.
 */
template<class T1, class T2,
         class S1 = NaiveSerializer<T1>,
         class S2 = NaiveSerializer<T2>>
class MapSerializer : public Serializer<std::map<T1, T2>> {

private:
  S1 s1;
  S2 s2;

public:
  void serialize(std::ostream &out, const std::map<T1, T2> &obj) {
    NaiveSerializer<size_t>().serialize(out, obj.size());
    for (const auto &p : obj) {
      s1.serialize(out, p.first);
      s2.serialize(out, p.second);
    }
  }

  std::map<T1, T2> deserialize(std::istream &in) {
    std::map<T1, T2> result;
    size_t size = NaiveSerializer<size_t>().deserialize(in);
    for (size_t i = 0; i < size; i++) {
      T1 key = s1.deserialize(in);
      T2 value = s2.deserialize(in);
      result[key] = value;
    }
    return result;
  }
};

/**
 * \brief Serializer template class for std::vector<T>.
 * Class S is serializer for class T.
 */
template<class T, class S = NaiveSerializer<T> >
class VectorSerializer : public Serializer<std::vector<T>> {

private:
  S s;

public:
  void serialize(std::ostream &out, const std::vector<T> &obj) {
    NaiveSerializer<size_t>().serialize(out, obj.size());
    for (const auto &i : obj) {
      s.serialize(out, i);
    }
  }

  std::vector<T> deserialize(std::istream &in) {
    std::vector<T> result;
    size_t size = NaiveSerializer<size_t>().deserialize(in);
    for (size_t i = 0; i < size; i++) {
      result.push_back(s.deserialize(in));
    }
    return result;
  }
};

} // namespace eda::util
