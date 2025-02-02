/**
 * @copyright (C) 2012-2020 Euclid Science Ground Segment
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3.0 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "EleFitsValidation/Benchmark.h"

#include <boost/test/unit_test.hpp>

using namespace Euclid::Fits;

struct ParamBenchmark : Test::Benchmark {
  ParamBenchmark(const std::string& filename, int i, double d) : Test::Benchmark(filename), m_i(i), m_d(d) {}
  void open() {};
  void close() {};
  int m_i;
  double m_d;
};

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Benchmark_test)

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(factory_test) {

  Test::BenchmarkFactory factory;
  factory.registerBenchmark<ParamBenchmark>("0, 3.14", 0, 3.14);
  factory.registerBenchmark<ParamBenchmark>("-1, 0.", -1, 0.);

  const auto b0 = factory.createBenchmark("0, 3.14", "file.fits");
  const auto* pb0 = dynamic_cast<const ParamBenchmark*>(b0.get());
  BOOST_TEST(pb0->m_i == 0);
  BOOST_TEST(pb0->m_d == 3.14);

  const auto b1 = factory.createBenchmark("-1, 0.", "file.fits");
  const auto* pb1 = dynamic_cast<const ParamBenchmark*>(b1.get());
  BOOST_TEST(pb1->m_i == -1);
  BOOST_TEST(pb1->m_d == 0.);
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()
