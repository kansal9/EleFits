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

#ifndef _EL_CFITSIOWRAPPER_RECORDHANDLER_H
#define _EL_CFITSIOWRAPPER_RECORDHANDLER_H

#include <boost/any.hpp>
#include <fitsio.h>
#include <string>
#include <tuple>
#include <vector>

#include "EL_FitsData/Record.h"

#include "EL_CfitsioWrapper/CfitsioUtils.h"
#include "EL_CfitsioWrapper/ErrorWrapper.h"
#include "EL_CfitsioWrapper/HduWrapper.h"
#include "EL_CfitsioWrapper/TypeWrapper.h"

namespace Euclid {
namespace Cfitsio {

/**
 * @brief Header-related functions.
 */
namespace Header {

/**
 * @brief List all the keywords.
 */
std::vector<std::string> listKeywords(fitsfile *fptr);

/**
 * @brief Parse a record.
 */
template <typename T>
FitsIO::Record<T> parseRecord(fitsfile *fptr, const std::string &keyword);

/**
 * @brief Parse records.
 */
template <typename... Ts>
std::tuple<FitsIO::Record<Ts>...> parseRecords(fitsfile *fptr, const std::vector<std::string> &keywords);

/**
 * @brief Parse records and store them in a user-defined structure.
 * @tparam TReturn A class which can be brace-initialized with a pack of records or values.
 */
template <class TReturn, typename... Ts>
TReturn parseRecordsAs(fitsfile *fptr, const std::vector<std::string> &keywords);

/**
 * @brief Write a new record.
 */
template <typename T>
void writeRecord(fitsfile *fptr, const FitsIO::Record<T> &record);

/**
 * @brief Write new records.
 */
template <typename... Ts>
void writeRecords(fitsfile *fptr, const FitsIO::Record<Ts> &... records);

/**
 * @brief Write new records.
 */
template <typename... Ts>
void writeRecords(fitsfile *fptr, const std::tuple<FitsIO::Record<Ts>...> &records);

/**
 * @brief Update an existing record or write a new one.
 */
template <typename T>
void updateRecord(fitsfile *fptr, const FitsIO::Record<T> &record);

/**
 * @brief Update existing records or write new ones.
 */
template <typename... Ts>
void updateRecords(fitsfile *fptr, const FitsIO::Record<Ts> &... records);

/**
 * @brief Update existing records or write new ones.
 */
template <typename... Ts>
void updateRecords(fitsfile *fptr, const std::tuple<FitsIO::Record<Ts>...> &records);

/**
 * @brief Delete an existing record.
 */
void deleteRecord(fitsfile *fptr, const std::string &keyword);

/**
 * @brief Get the CFitsIO type code of a record.
 */
int recordTypecode(fitsfile *fptr, const std::string &keyword);

///////////////
// INTERNAL //
/////////////

/// @cond INTERNAL
namespace Internal {

// Signature change (output argument) for further use with variadic templates.
template <typename T>
inline void parseRecordImpl(fitsfile *fptr, const std::string &keyword, FitsIO::Record<T> &record) {
  record = parseRecord<T>(fptr, keyword);
}

// Parse the records of the i+1 first keywords of a given list (recursive approach).
template <std::size_t i, typename... Ts>
struct ParseRecordsImpl {
  void
  operator()(fitsfile *fptr, const std::vector<std::string> &keywords, std::tuple<FitsIO::Record<Ts>...> &records) {
    parseRecordImpl(fptr, keywords[i], std::get<i>(records));
    ParseRecordsImpl<i - 1, Ts...> {}(fptr, keywords, records);
  }
};

// Parse the value of the first keyword of a given list (terminal case of the recursion).
template <typename... Ts>
struct ParseRecordsImpl<0, Ts...> {
  void
  operator()(fitsfile *fptr, const std::vector<std::string> &keywords, std::tuple<FitsIO::Record<Ts>...> &records) {
    parseRecordImpl(fptr, keywords[0], std::get<0>(records));
  }
};

template <class TReturn, typename... Ts, std::size_t... Is>
TReturn parseRecordsAsImpl(fitsfile *fptr, const std::vector<std::string> &keywords, std14::index_sequence<Is...>) {
  return { parseRecord<Ts>(fptr, keywords[Is])... };
}

template <typename... Ts, std::size_t... Is>
void writeRecordsImpl(fitsfile *fptr, const std::tuple<FitsIO::Record<Ts>...> &records, std14::index_sequence<Is...>) {
  using mockUnpack = int[];
  (void)mockUnpack { (writeRecord<Ts>(fptr, std::get<Is>(records)), 0)... };
}

template <typename... Ts, std::size_t... Is>
void updateRecordsImpl(fitsfile *fptr, const std::tuple<FitsIO::Record<Ts>...> &records, std14::index_sequence<Is...>) {
  using mockUnpack = int[];
  (void)mockUnpack { (updateRecord<Ts>(fptr, std::get<Is>(records)), 0)... };
}

} // namespace Internal
/// @endcond

/////////////////////
// IMPLEMENTATION //
///////////////////

template <typename T>
FitsIO::Record<T> parseRecord(fitsfile *fptr, const std::string &keyword) {
  int status = 0;
  /* Read value and comment */
  T value;
  char comment[FLEN_COMMENT];
  comment[0] = '\0';
  fits_read_key(fptr, TypeCode<T>::forRecord(), keyword.c_str(), &value, comment, &status);
  /* Read unit */
  char unit[FLEN_COMMENT];
  unit[0] = '\0';
  fits_read_key_unit(fptr, keyword.c_str(), unit, &status);
  std::string context = "while parsing '" + keyword + "' in HDU #" + std::to_string(Hdu::currentIndex(fptr));
  mayThrowCfitsioError(status, context);
  /* Build Record */
  FitsIO::Record<T> record(keyword, value, std::string(unit), std::string(comment));
  /* Separate comment and unit */
  if (record.comment == record.unit) {
    record.comment == "";
  } else if (record.unit != "") {
    std::string match = "[" + record.unit + "] ";
    auto pos = record.comment.find(match);
    if (pos != std::string::npos) {
      record.comment.erase(pos, match.length());
    }
  }
  return record;
}

template <>
FitsIO::Record<std::string> parseRecord<std::string>(fitsfile *fptr, const std::string &keyword);

template <>
FitsIO::Record<boost::any> parseRecord<boost::any>(fitsfile *fptr, const std::string &keyword);

template <typename... Ts>
std::tuple<FitsIO::Record<Ts>...> parseRecords(fitsfile *fptr, const std::vector<std::string> &keywords) {
  std::tuple<FitsIO::Record<Ts>...> records;
  Internal::ParseRecordsImpl<sizeof...(Ts) - 1, Ts...> {}(fptr, keywords, records);
  return records;
}

template <class Return, typename... Ts>
Return parseRecordsAs(fitsfile *fptr, const std::vector<std::string> &keywords) {
  return Internal::parseRecordsAsImpl<Return, Ts...>(fptr, keywords, std14::make_index_sequence<sizeof...(Ts)>());
}

template <typename T>
void writeRecord(fitsfile *fptr, const FitsIO::Record<T> &record) {
  int status = 0;
  std::string comment = record.comment;
  T value = record.value;
  fits_write_key(fptr, TypeCode<T>::forRecord(), record.keyword.c_str(), &value, &comment[0], &status);
  fits_write_key_unit(fptr, record.keyword.c_str(), record.unit.c_str(), &status);
  std::string context = "while writing '" + record.keyword + "' in HDU #" + std::to_string(Hdu::currentIndex(fptr));
  mayThrowCfitsioError(status, context);
}

template <>
void writeRecord<std::string>(fitsfile *fptr, const FitsIO::Record<std::string> &record);

template <>
void writeRecord<const char *>(fitsfile *fptr, const FitsIO::Record<const char *> &record);

template <typename... Ts>
void writeRecords(fitsfile *fptr, const FitsIO::Record<Ts> &... records) {
  using mockUnpack = int[];
  (void)mockUnpack { (writeRecord(fptr, records), 0)... };
}

template <typename... Ts>
void writeRecords(fitsfile *fptr, const std::tuple<FitsIO::Record<Ts>...> &records) {
  Internal::writeRecordsImpl(fptr, records, std14::make_index_sequence<sizeof...(Ts)>());
}

template <typename T>
void updateRecord(fitsfile *fptr, const FitsIO::Record<T> &record) {
  int status = 0;
  std::string comment = record.comment;
  T value = record.value;
  fits_update_key(fptr, TypeCode<T>::forRecord(), record.keyword.c_str(), &value, &comment[0], &status);
  std::string context = "while updating '" + record.keyword + "' in HDU #" + std::to_string(Hdu::currentIndex(fptr));
  mayThrowCfitsioError(status, context);
}

template <>
void updateRecord<std::string>(fitsfile *fptr, const FitsIO::Record<std::string> &record);

template <>
void updateRecord<const char *>(fitsfile *fptr, const FitsIO::Record<const char *> &record);

template <typename... Ts>
void updateRecords(fitsfile *fptr, const FitsIO::Record<Ts> &... records) {
  using mockUnpack = int[];
  (void)mockUnpack { (updateRecord(fptr, records), 0)... };
}

template <typename... Ts>
void updateRecords(fitsfile *fptr, const std::tuple<FitsIO::Record<Ts>...> &records) {
  Internal::updateRecordsImpl(fptr, records, std14::make_index_sequence<sizeof...(Ts)>());
}

} // namespace Header
} // namespace Cfitsio
} // namespace Euclid

#endif
