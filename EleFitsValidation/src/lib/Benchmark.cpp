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

namespace Euclid {
namespace Fits {
namespace Test {

Benchmark::Benchmark(const std::string& filename) :
    m_filename(filename), m_chrono(), m_logger(Elements::Logging::getLogger("Benchmark")) {}

const BChronometer& Benchmark::writeImages(long count, const BRaster& raster) {
  open();
  m_chrono.reset();
  m_logger.debug() << "First pixel: " << raster.at({ 0 });
  m_logger.debug() << "Last pixel: " << raster.at({ -1 });
  for (long i = 0; i < count; ++i) {
    const auto inc = writeImage(raster);
    m_logger.debug() << i + 1 << "/" << count << ": " << inc.count() << "ms";
  }
  const auto total = m_chrono.elapsed();
  m_logger.debug() << "TOTAL: " << total.count() << "ms";
  close();
  return m_chrono;
}

const BChronometer& Benchmark::writeBintables(long count, const BColumns& columns) { // TODO avoid duplication
  open();
  m_chrono.reset();
  m_logger.debug() << "First column, first row: " << std::get<0>(columns).at(0, 0);
  m_logger.debug() << "Last column, last row: " << std::get<columnCount - 1>(columns).at(-1, -1);
  for (long i = 0; i < count; ++i) {
    const auto inc = writeBintable(columns);
    m_logger.debug() << i + 1 << "/" << count << ": " << inc.count() << "ms";
  }
  const auto total = m_chrono.elapsed();
  m_logger.debug() << "TOTAL: " << total.count() << "ms";
  close();
  return m_chrono;
}

const BChronometer& Benchmark::readImages(long first, long count) {
  open();
  m_chrono.reset();
  for (long i = 0; i < count; ++i) {
    const auto raster = readImage(first + i);
    m_logger.debug() << i + 1 << "/" << count << ": " << m_chrono.last().count() << "ms";
    m_logger.debug() << "\tFirst pixel: " << raster.at({ 0 });
    m_logger.debug() << "\tLast pixel: " << raster.at({ -1 });
  }
  const auto total = m_chrono.elapsed();
  m_logger.debug() << "TOTAL: " << total.count() << "ms";
  close();
  return m_chrono;
}

const BChronometer& Benchmark::readBintables(long first, long count) {
  open();
  m_chrono.reset();
  for (long i = 0; i < count; ++i) {
    const auto columns = readBintable(i + first);
    m_logger.debug() << i + 1 << "/" << count << ": " << m_chrono.last().count() << "ms";
    m_logger.debug() << "\tFirst column, first row: " << std::get<0>(columns).at(0, 0);
    m_logger.debug() << "\tLast column, last row: " << std::get<columnCount - 1>(columns).at(-1, -1);
  }
  const auto total = m_chrono.elapsed();
  m_logger.debug() << "TOTAL: " << total.count() << "ms";
  close();
  return m_chrono;
}

void BenchmarkFactory::registerBenchmarkMaker(const std::string& key, BenchmarkMaker factory) {
  if (m_register.find(key) != m_register.end()) {
    throw std::runtime_error(std::string("Benchmark already registered: ") + key);
  }
  m_register[key] = factory;
}

std::unique_ptr<Benchmark>
BenchmarkFactory::createBenchmark(const std::string& key, const std::string& filename) const {
  const auto it = m_register.find(key);
  if (it == m_register.end()) {
    throw TestCaseNotImplemented(key);
  }
  return it->second(filename);
}

std::vector<std::string> BenchmarkFactory::keys() const {
  std::vector<std::string> res;
  for (const auto& kv : m_register) {
    res.push_back(kv.first);
  }
  return res;
}

} // namespace Test
} // namespace Fits
} // namespace Euclid
