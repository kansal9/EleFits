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

#ifndef _ELEFITS_VALIDATION_BENCHMARK_H
#define _ELEFITS_VALIDATION_BENCHMARK_H

#include "EleFitsData/Column.h"
#include "EleFitsData/DataUtils.h"
#include "EleFitsData/Raster.h"
#include "EleFitsValidation/Chronometer.h"
#include "ElementsKernel/Logging.h"

#include <memory>
#include <tuple>
#include <unordered_map>

namespace Euclid {
namespace Fits {
namespace Test {

/**
 * @brief The raster type used for benchmarking.
 */
using BRaster = VecRaster<std::int64_t, 1>;

/**
 * @brief The column types used for benchmarking.
 */
using BColumns = std::tuple<
    VecColumn<unsigned char>,
    VecColumn<std::int32_t>,
    VecColumn<std::int64_t>,
    VecColumn<float>,
    VecColumn<double>,
    VecColumn<std::complex<float>>,
    VecColumn<std::complex<double>>,
    // VecColumn<std::string>, //TODO to be handled specially
    VecColumn<char>,
    VecColumn<std::uint32_t>,
    VecColumn<std::uint64_t>>;

/**
 * @brief The number of columns.
 */
constexpr std::size_t columnCount = std::tuple_size<BColumns>::value;

/**
 * @brief The chronometer used for benchmarking.
 */
using BChronometer = Chronometer<std::chrono::milliseconds>;

/**
 * @brief The exception which is thrown when a test case is not implemented.
 */
struct TestCaseNotImplemented : public std::exception {

  /**
   * @brief Constructor.
   */
  explicit TestCaseNotImplemented(const std::string& testCaseName) :
      message("Test case not implemented: " + testCaseName) {}

  /**
   * @brief Get the error message.
   */
  virtual const char* what() const noexcept override {
    return message.c_str();
  }

  /**
   * @brief The error message.
   */
  std::string message;
};

/**
 * @brief The base class for all test cases of the benchmark.
 */
class Benchmark {
public:
  /**
   * @brief Destructor.
   */
  virtual ~Benchmark() = default;

  /**
   * @brief Constructor.
   */
  Benchmark(const std::string& filename);

  /**
   * @brief Open file.
   */
  virtual void open() = 0;

  /**
   * @brief Close file.
   */
  virtual void close() = 0;

  /**
   * @brief Write the given raster in new image extensions.
   * @param count The number of HDUs
   * @param raster The raster to be written in each HDU
   */
  const BChronometer& writeImages(long count, const BRaster& raster);

  /**
   * @brief Write the given columns in new binary table extensions.
   * @param count The number of HDUs
   * @param columns The columns to be written in each HDU
   */
  const BChronometer& writeBintables(long count, const BColumns& columns);

  /**
   * @brief Read the rasters in the given image extensions.
   * @param first The first (0-based) HDU index
   * @param count The number of HDUs
   */
  const BChronometer& readImages(long first, long count);

  /**
   * @brief Read the columns in the given binary table extensions.
   * @param first The first (0-based) HDU index
   * @param count The number of HDUs
   */
  const BChronometer& readBintables(long first, long count);

  /**
   * @brief Write the given raster in a new image extension.
   * @details
   * This method is implemented by the child classes when part of the test case.
   * They have to manage the internal chronometer by calling m_chrono.start() and m_chrono.stop() at the right place.
   */
  virtual BChronometer::Unit writeImage(const BRaster&) {
    throw TestCaseNotImplemented("Write image");
  }

  /**
   * @brief Write the given columns in a new binary table extension.
   * @copydetails writeImage
   */
  virtual BChronometer::Unit writeBintable(const BColumns&) {
    throw TestCaseNotImplemented("Write binary table");
  }

  /**
   * @brief Read the image raster in the given image extension.
   * @copydetails writeImage
   */
  virtual BRaster readImage(long) {
    throw TestCaseNotImplemented("Read image");
  }

  /**
   * @brief Read the columns in the given binary table extension.
   * @copydetails writeImage
   */
  virtual BColumns readBintable(long) {
    throw TestCaseNotImplemented("Read binary table");
  }

protected:
  /** @brief The file name. */
  std::string m_filename;
  /** @brief The chronometer. */
  BChronometer m_chrono;
  /** @brief The logger. */
  Elements::Logging m_logger;
};

/**
 * @brief Abstract factory for the Benchmark child classes.
 */
class BenchmarkFactory {
public:
  /**
   * @brief The type of the benchmark factory function.
   * @details
   * The factory function takes a filename as input,
   * and returns a pointer to an instance of the benchmark child class.
   */
  using BenchmarkMaker = std::function<std::unique_ptr<Benchmark>(const std::string&)>;

  /**
   * @brief Register a new benchmark with given key and factory function.
   */
  void registerBenchmarkMaker(const std::string& key, BenchmarkMaker factory);

  /**
   * @brief Register a new benchmark with given key and constructor arguments.
   * @details
   * The benchmark will be created as:
   * \code
   * TBenchmark(filename, args...);
   * \endcode
   */
  template <typename TBenchmark, typename... Ts>
  void registerBenchmark(const std::string& key, const Ts&... args) {
    registerBenchmarkMaker(key, [=](const std::string& filename) { // Force copy
      return std::make_unique<TBenchmark>(filename, args...);
    });
  }

  /**
   * @brief Create a new benchmark from its name and filename.
   */
  std::unique_ptr<Benchmark> createBenchmark(const std::string& key, const std::string& filename) const;

  /**
   * @brief Get the registered keys.
   */
  std::vector<std::string> keys() const;

private:
  std::unordered_map<std::string, BenchmarkMaker> m_register;
};

} // namespace Test
} // namespace Fits
} // namespace Euclid

#endif
