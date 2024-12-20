//
//  zipfian_generator.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/7/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef _GENERATOR_HPP_
#define _GENERATOR_HPP_

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <random>
#include "workload_utilis.hpp"

template <typename Value>
class Generator {
 public:
  virtual Value Next() = 0;
  virtual Value Last() = 0;
  virtual ~Generator() {}
};

class ZipfianGenerator : public Generator<uint64_t> {
 public:
  constexpr static const double kZipfianConst = 0.99;
  static const uint64_t kMaxNumItems = (UINT64_MAX >> 24);

  ZipfianGenerator(uint64_t min, uint64_t max,
                   double zipfian_const = kZipfianConst)
      : num_items_(max - min + 1),
        base_(min),
        theta_(zipfian_const),
        zeta_n_(0),
        n_for_zeta_(0) {
    assert(num_items_ >= 2 && num_items_ < kMaxNumItems);
    zeta_2_ = Zeta(2, theta_);
    alpha_ = 1.0 / (1.0 - theta_);
    RaiseZeta(num_items_);
    eta_ = Eta();

    Next();
  }

  ZipfianGenerator(uint64_t num_items)
      : ZipfianGenerator(0, num_items - 1, kZipfianConst) {}

  uint64_t Next(uint64_t num_items);

  uint64_t Next() { return Next(num_items_); }

  uint64_t Last();

 private:
  ///
  /// Compute the zeta constant needed for the distribution.
  /// Remember the number of items, so if it is changed, we can recompute zeta.
  ///
  void RaiseZeta(uint64_t num) {
    assert(num >= n_for_zeta_);
    zeta_n_ = Zeta(n_for_zeta_, num, theta_, zeta_n_);
    n_for_zeta_ = num;
  }

  double Eta() {
    return (1 - std::pow(2.0 / num_items_, 1 - theta_)) /
           (1 - zeta_2_ / zeta_n_);
  }

  ///
  /// Calculate the zeta constant needed for a distribution.
  /// Do this incrementally from the last_num of items to the cur_num.
  /// Use the zipfian constant as theta. Remember the new number of items
  /// so that, if it is changed, we can recompute zeta.
  ///
  static double Zeta(uint64_t last_num, uint64_t cur_num, double theta,
                     double last_zeta) {
    double zeta = last_zeta;
    for (uint64_t i = last_num + 1; i <= cur_num; ++i) {
      zeta += 1 / std::pow(i, theta);
    }
    return zeta;
  }

  static double Zeta(uint64_t num, double theta) {
    return Zeta(0, num, theta, 0);
  }

  uint64_t num_items_;
  uint64_t base_;  /// Min number of items to generate

  // Computed parameters for generating the distribution
  double theta_, zeta_n_, eta_, alpha_, zeta_2_;
  uint64_t n_for_zeta_;  /// Number of items used to compute zeta_n
  uint64_t last_value_;
  std::mutex mutex_;
};

inline uint64_t ZipfianGenerator::Next(uint64_t num) {
  assert(num >= 2 && num < kMaxNumItems);
  std::lock_guard<std::mutex> lock(mutex_);

  if (num > n_for_zeta_) {  // Recompute zeta_n and eta
    RaiseZeta(num);
    eta_ = Eta();
  }

  double u = utils::RandomDouble();
  double uz = u * zeta_n_;

  if (uz < 1.0) {
    return last_value_ = 0;
  }

  if (uz < 1.0 + std::pow(0.5, theta_)) {
    return last_value_ = 1;
  }

  return last_value_ = base_ + num * std::pow(eta_ * u - eta_ + 1, alpha_);
}

inline uint64_t ZipfianGenerator::Last() {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_value_;
}

class CounterGenerator : public Generator<uint64_t> {
 public:
  CounterGenerator(uint64_t start) : counter_(start) {}
  uint64_t Next() { return counter_.fetch_add(1); }
  uint64_t Last() { return counter_.load() - 1; }
  void Set(uint64_t start) { counter_.store(start); }

 private:
  std::atomic<uint64_t> counter_;
};

template <typename Value>
class DiscreteGenerator : public Generator<Value> {
 public:
  DiscreteGenerator() : sum_(0) {}
  void AddValue(Value value, double weight);

  Value Next();
  Value Last() { return last_; }

 private:
  std::vector<std::pair<Value, double>> values_;
  double sum_;
  std::atomic<Value> last_;
  std::mutex mutex_;
};

template <typename Value>
inline void DiscreteGenerator<Value>::AddValue(Value value, double weight) {
  if (values_.empty()) {
    last_ = value;
  }
  values_.push_back(std::make_pair(value, weight));
  sum_ += weight;
}

template <typename Value>
inline Value DiscreteGenerator<Value>::Next() {
  mutex_.lock();
  double chooser = utils::RandomDouble();
  mutex_.unlock();

  for (auto p = values_.cbegin(); p != values_.cend(); ++p) {
    if (chooser < p->second / sum_) {
      return last_ = p->first;
    }
    chooser -= p->second / sum_;
  }

  assert(false);
  return last_;
}

class UniformGenerator : public Generator<uint64_t> {
 public:
  // Both min and max are inclusive
  UniformGenerator(uint64_t min, uint64_t max) : dist_(min, max) { Next(); }

  uint64_t Next();
  uint64_t Last();

 private:
  std::mt19937_64 generator_;
  std::uniform_int_distribution<uint64_t> dist_;
  uint64_t last_int_;
  std::mutex mutex_;
};

inline uint64_t UniformGenerator::Next() {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_int_ = dist_(generator_);
}

inline uint64_t UniformGenerator::Last() {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_int_;
}

#endif  // _GENERATOR_HPP_
