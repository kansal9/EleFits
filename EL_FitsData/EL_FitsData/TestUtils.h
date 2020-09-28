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

#ifndef _EL_FITSDATA_TESTUTILS_H
#define _EL_FITSDATA_TESTUTILS_H

#include <complex>
#include <string>
#include <vector>

namespace Euclid {
namespace FitsIO {

/**
 * @brief Test-related classes and functions.
 */
namespace Test {

/**
 * @brief Value very close to the min of the type.
 */
template <typename T>
T almostMin();

template <>
std::complex<float> almostMin<std::complex<float>>();

template <>
std::complex<double> almostMin<std::complex<double>>();

template <>
std::string almostMin<std::string>();

/**
 * @brief Value very close to the max of the type.
 */
template <typename T>
T almostMax();

template <>
std::complex<float> almostMax<std::complex<float>>();

template <>
std::complex<double> almostMax<std::complex<double>>();

template <>
std::string almostMax<std::string>();

/**
 * @brief Generate a random value of given type.
 */
template <typename T>
T generateRandomValue(T min = almostMin<T>(), T max = almostMax<T>());

/**
 * @brief Generate a random vector of given type and size.
 */
template <typename T>
std::vector<T> generateRandomVector(long size, T min = almostMin<T>(), T max = almostMax<T>());

/**
 * @brief Specialization of generateRandomVector for complex<float>.
 */
template <>
std::vector<std::complex<float>>
generateRandomVector<std::complex<float>>(long size, std::complex<float> min, std::complex<float> max);

/**
 * @brief Specialization of generateRandomVector for complex<double>.
 */
template <>
std::vector<std::complex<double>>
generateRandomVector<std::complex<double>>(long size, std::complex<double> min, std::complex<double> max);

/**
 * @brief Specialization of generateRandomVector for string.
 */
template <>
std::vector<std::string> generateRandomVector<std::string>(long size, std::string min, std::string max);

} // namespace Test
} // namespace FitsIO
} // namespace Euclid

#define _EL_FITSDATA_TESTUTILS_IMPL
#include "EL_FitsData/impl/TestUtils.hpp"
#undef _EL_FITSDATA_TESTUTILS_IMPL

#endif
