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

#ifndef _ELECFITSIOWRAPPER_TYPEWRAPPER_H
#define _ELECFITSIOWRAPPER_TYPEWRAPPER_H

#include <complex>
#include <fitsio.h>
#include <string>
#include <vector>

namespace Euclid {
namespace Cfitsio {

/**
 * @brief Type traits to convert C++ types to CFitsIO type codes.
 * @details
 * Used to read and write:
 * * Records,
 * * Images,
 * * Binary tables (ASCII table not supported).
 */
template <typename T>
struct TypeCode {

  /**
   * @brief Get the type code for a record.
   */
  inline static int forRecord();

  /**
   * @brief Get the type code for a binary table.
   */
  inline static int forBintable();

  /**
   * @brief Get the TFORM value to handle binary table columns.
   */
  inline static std::string tform(long repeatCount);

  /**
   * @brief Get the type code for an image.
   */
  inline static int forImage();

  /**
   * @brief Get the BITPIX value to handle images.
   */
  inline static int bitpix();
};

} // namespace Cfitsio
} // namespace Euclid

/// @cond INTERNAL
#define _ELECFITSIOWRAPPER_TYPEWRAPPER_IMPL
#include "EleCfitsioWrapper/impl/TypeWrapper.hpp"
#undef _ELECFITSIOWRAPPER_TYPEWRAPPER_IMPL
/// @endcond

#endif
