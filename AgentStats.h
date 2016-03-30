/* -*- c++ -*- */
#ifndef AGENTSTATS_H
#define AGENTSTATS_H

#include "LogHistogramSampler.h"

class AgentStats {
public:
  uint64_t rx_bytes, tx_bytes;
  uint64_t gets, sets, get_misses;
  uint64_t skips;
  uint64_t get_bins[LOGSAMPLER_BINS];
  double get_sum;
  double get_sum_sq;

  double start, stop;
};

#endif // AGENTSTATS_H
