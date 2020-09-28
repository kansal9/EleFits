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

#ifdef _EL_FITSDATA_TESTCOLUMN_IMPL

#include <algorithm>

#include "EL_FitsData/TestUtils.h"

#include "EL_FitsData/TestColumn.h"

namespace Euclid {
namespace FitsIO {
namespace Test {

template <typename T>
RandomScalarColumn<T>::RandomScalarColumn(long size, T min, T max) :
    VecColumn<T>({ "SCALAR", "m", 1 }, generateRandomVector<T>(size, min, max)) {
}

template <>
RandomScalarColumn<std::string>::RandomScalarColumn(long size, std::string min, std::string max) :
    VecColumn<std::string>({ "SCALAR", "m", 1 }, generateRandomVector<std::string>(size, min, max)) {
  for (const auto &v : vector()) {
    long currentSize = static_cast<long>(v.length() + 1); // +1 for '\0'
    if (currentSize > info.repeat) {
      info.repeat = currentSize;
    }
  }
}

template <typename T>
SmallVectorColumn<T>::SmallVectorColumn() :
    VecColumn<std::vector<T>>({ "VECTOR", "m2", 2 }, { { T(0.), T(1.) }, { T(2.), T(3.) }, { T(4.), T(5.) } }) {
}

} // namespace Test
} // namespace FitsIO
} // namespace Euclid

#endif
