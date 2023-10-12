// boost accumulator for decoding function
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>

// record time
#include <chrono>

typedef boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::p_square_quantile> > accumulator_t;
extern accumulator_t decoder_time_acc_25;
extern accumulator_t decoder_time_acc_50;
extern accumulator_t decoder_time_acc_75;
extern accumulator_t decoder_time_acc_99;
extern accumulator_t dematcher_time_acc_25;
extern accumulator_t dematcher_time_acc_50;
extern accumulator_t dematcher_time_acc_75;
extern accumulator_t dematcher_time_acc_99;