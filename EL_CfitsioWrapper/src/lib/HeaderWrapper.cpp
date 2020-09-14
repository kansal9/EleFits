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

#include <limits>

#include "EL_CfitsioWrapper/HeaderWrapper.h"

namespace Euclid {
namespace Cfitsio {
namespace Header {

std::vector<std::string> listKeywords(fitsfile *fptr) {
  int count = 0;
  int status = 0;
  fits_get_hdrspace(fptr, &count, nullptr, &status);
  std::vector<std::string> keywords(count);
  char keyword[FLEN_KEYWORD];
  char value[FLEN_KEYWORD];
  for (int i = 0; i < count; ++i) {
    fits_read_keyn(fptr, i + 1, keyword, value, nullptr, &status);
    keywords[i].assign(keyword);
  }
  return keywords;
}

template <>
FitsIO::Record<std::string> parseRecord<std::string>(fitsfile *fptr, const std::string &keyword) {
  int status = 0;
  int length = 0;
  fits_get_key_strlen(fptr, keyword.c_str(), &length, &status);
  mayThrowCfitsioError(status, "Cannot find string keyword " + keyword);
  if (length == 0) {
    return { keyword, "" };
  }
  char *value = nullptr; // That's the only function in which CFitsIO allocates itself!
  char unit[FLEN_COMMENT];
  unit[0] = '\0';
  char comment[FLEN_COMMENT];
  comment[0] = '\0';
  fits_read_key_longstr(fptr, keyword.c_str(), &value, comment, &status);
  fits_read_key_unit(fptr, keyword.c_str(), unit, &status);
  std::string str_value(value);
  if (status == VALUE_UNDEFINED) {
    str_value = "";
    status = 0;
  }
  FitsIO::Record<std::string> record(keyword, str_value, std::string(unit), std::string(comment));
  free(value);
  std::string context = "while parsing '" + keyword + "' in HDU #" + std::to_string(Hdu::currentIndex(fptr));
  mayThrowCfitsioError(status, context);
  mayThrowCfitsioError(status, context);
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
FitsIO::Record<boost::any> parseRecord<boost::any>(fitsfile *fptr, const std::string &keyword) {
  const auto code = recordTypecode(fptr, keyword);
  switch (code) {
    case TLOGICAL:
      return parseRecord<bool>(fptr, keyword);
    case TSBYTE:
      return parseRecord<char>(fptr, keyword);
    case TSHORT:
      return parseRecord<short>(fptr, keyword);
    case TINT:
      return parseRecord<int>(fptr, keyword);
    case TLONG:
      return parseRecord<long>(fptr, keyword);
    case TLONGLONG:
      return parseRecord<long long>(fptr, keyword);
    case TFLOAT:
      return parseRecord<float>(fptr, keyword);
    case TDOUBLE:
      return parseRecord<double>(fptr, keyword);
    case TCOMPLEX:
      return parseRecord<std::complex<float>>(fptr, keyword);
    case TDBLCOMPLEX:
      return parseRecord<std::complex<double>>(fptr, keyword);
    case TSTRING:
      return parseRecord<std::string>(fptr, keyword);
    case TBYTE:
      return parseRecord<unsigned char>(fptr, keyword);
    case TUSHORT:
      return parseRecord<unsigned short>(fptr, keyword);
    case TUINT:
      return parseRecord<unsigned int>(fptr, keyword);
    case TULONG:
      return parseRecord<unsigned long>(fptr, keyword);
    case TULONGLONG:
      return parseRecord<unsigned long long>(fptr, keyword);
    default:
      throw std::runtime_error("Cannot deduce type for keyword: " + keyword);
  }
}

template <>
void writeRecord<std::string>(fitsfile *fptr, const FitsIO::Record<std::string> &record) {
  int status = 0;
  if (record.value.length() > 68) { // https://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/c_user/node118.html
    fits_write_key_longwarn(fptr, &status);
  }
  fits_write_key_longstr(fptr, record.keyword.c_str(), &std::string(record.value)[0], &record.comment[0], &status);
  fits_write_key_unit(fptr, record.keyword.c_str(), record.unit.c_str(), &status);
  mayThrowCfitsioError(status, "Cannot write record: " + record.keyword);
}

template <>
void writeRecord<const char *>(fitsfile *fptr, const FitsIO::Record<const char *> &record) {
  int status = 0;
  if (strlen(record.value) > 68) { // https://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/c_user/node118.html
    fits_write_key_longwarn(fptr, &status);
  }
  fits_write_key_longstr(fptr, record.keyword.c_str(), record.value, &record.comment[0], &status);
  fits_write_key_unit(fptr, record.keyword.c_str(), record.unit.c_str(), &status);
  mayThrowCfitsioError(status, "Cannot write record: " + record.keyword);
}

template <>
void writeRecord<boost::any>(fitsfile *fptr, const FitsIO::Record<boost::any> &record) {
  const auto &id = record.value.type();
  if (id == typeid(bool)) {
    writeRecord<bool>(fptr, record.keyword);
  } else if (id == typeid(char)) {
    writeRecord<char>(fptr, record.keyword);
  } else if (id == typeid(short)) {
    writeRecord<short>(fptr, record.keyword);
  } else if (id == typeid(int)) {
    writeRecord<int>(fptr, record.keyword);
  } else if (id == typeid(long)) {
    writeRecord<long>(fptr, record.keyword);
  } else if (id == typeid(long long)) {
    writeRecord<long long>(fptr, record.keyword);
  } else if (id == typeid(float)) {
    writeRecord<float>(fptr, record.keyword);
  } else if (id == typeid(double)) {
    writeRecord<double>(fptr, record.keyword);
  } else if (id == typeid(std::complex<float>)) {
    writeRecord<std::complex<float>>(fptr, record.keyword);
  } else if (id == typeid(std::complex<double>)) {
    writeRecord<std::complex<double>>(fptr, record.keyword);
  } else if (id == typeid(std::string)) {
    writeRecord<std::string>(fptr, record.keyword);
  } else if (id == typeid(unsigned char)) {
    writeRecord<unsigned char>(fptr, record.keyword);
  } else if (id == typeid(unsigned short)) {
    writeRecord<unsigned short>(fptr, record.keyword);
  } else if (id == typeid(unsigned int)) {
    writeRecord<unsigned int>(fptr, record.keyword);
  } else if (id == typeid(unsigned long)) {
    writeRecord<unsigned long>(fptr, record.keyword);
  } else if (id == typeid(unsigned long long)) {
    writeRecord<unsigned long long>(fptr, record.keyword);
  } else {
    throw std::runtime_error("Cannot deduce type for record: " + record.keyword);
  }
}

template <>
void updateRecord<std::string>(fitsfile *fptr, const FitsIO::Record<std::string> &record) {
  int status = 0;
  fits_update_key(
      fptr,
      TypeCode<std::string>::forRecord(),
      record.keyword.c_str(),
      &std::string(record.value)[0],
      &record.comment[0],
      &status);
  mayThrowCfitsioError(status, "Cannot update record: " + record.keyword);
}

template <>
void updateRecord<const char *>(fitsfile *fptr, const FitsIO::Record<const char *> &record) {
  int status = 0;
  fits_update_key(
      fptr,
      TypeCode<std::string>::forRecord(),
      record.keyword.c_str(),
      &std::string(record.value)[0],
      &record.comment[0],
      &status);
  mayThrowCfitsioError(status, "Cannot update record: " + record.keyword);
}

template <>
void updateRecord<boost::any>(fitsfile *fptr, const FitsIO::Record<boost::any> &record) {
  const auto &id = record.value.type();
  if (id == typeid(bool)) {
    updateRecord<bool>(fptr, record.keyword);
  } else if (id == typeid(char)) {
    updateRecord<char>(fptr, record.keyword);
  } else if (id == typeid(short)) {
    updateRecord<short>(fptr, record.keyword);
  } else if (id == typeid(int)) {
    updateRecord<int>(fptr, record.keyword);
  } else if (id == typeid(long)) {
    updateRecord<long>(fptr, record.keyword);
  } else if (id == typeid(long long)) {
    updateRecord<long long>(fptr, record.keyword);
  } else if (id == typeid(float)) {
    updateRecord<float>(fptr, record.keyword);
  } else if (id == typeid(double)) {
    updateRecord<double>(fptr, record.keyword);
  } else if (id == typeid(std::complex<float>)) {
    updateRecord<std::complex<float>>(fptr, record.keyword);
  } else if (id == typeid(std::complex<double>)) {
    updateRecord<std::complex<double>>(fptr, record.keyword);
  } else if (id == typeid(std::string)) {
    updateRecord<std::string>(fptr, record.keyword);
  } else if (id == typeid(unsigned char)) {
    updateRecord<unsigned char>(fptr, record.keyword);
  } else if (id == typeid(unsigned short)) {
    updateRecord<unsigned short>(fptr, record.keyword);
  } else if (id == typeid(unsigned int)) {
    updateRecord<unsigned int>(fptr, record.keyword);
  } else if (id == typeid(unsigned long)) {
    updateRecord<unsigned long>(fptr, record.keyword);
  } else if (id == typeid(unsigned long long)) {
    updateRecord<unsigned long long>(fptr, record.keyword);
  } else {
    throw std::runtime_error("Cannot deduce type for record: " + record.keyword);
  }
}

void deleteRecord(fitsfile *fptr, const std::string &keyword) {
  int status = 0;
  fits_delete_key(fptr, keyword.c_str(), &status);
  mayThrowCfitsioError(status, "Cannot delete record: " + keyword);
}

namespace Internal {

/**
 * @return
 * TSBYTE      -128 to 127
 * TBYTE        128 to 255
 * TSHORT      -32768 to 32767
 * TUSHORT      32768 to 65535
 * TINT        -2147483648 to 2147483647
 * TUINT        2147483648 to 4294967295
 * TLONGLONG   -9223372036854775808 to 9223372036854775807
 */
int intRecordTypecodeImpl(char *value) {
  int status = 0;
  int typecode = 0;
  int isNegative = 0;
  fits_get_inttype(value, &typecode, &isNegative, &status);
  mayThrowCfitsioError(status, "Cannot deduce integer type code of value: " + std::string(value));
  return typecode;
}

/**
 * @return TFLOAT, TDOUBLE.
 */
int floatRecordTypecodeImpl(char *value) {
  double parsed = std::atof(value);
  if (parsed < std::numeric_limits<float>::lowest()) {
    return TDOUBLE;
  }
  if (parsed > std::numeric_limits<float>::max()) {
    return TDOUBLE;
  }
  return TFLOAT;
}

/**
 * @return TCOMPLEX, TDBLCOMPLEX.
 */
int complexRecordTypecodeImpl(char *value) {
  const std::size_t re_begin = 1; // 1 for '('
  const std::size_t re_end = std::string(value).find(",");
  const std::size_t im_begin = re_end + 2; // 1 for ', '
  const std::size_t im_end = std::string(value).find(")");
  char re[FLEN_VALUE];
  strncpy(re, value + re_begin, re_end - re_begin);
  char im[FLEN_VALUE];
  strncpy(im, value + im_begin, im_end - im_begin);
  if (floatRecordTypecodeImpl(re) == TDOUBLE) {
    return TDBLCOMPLEX;
  }
  if (floatRecordTypecodeImpl(im) == TDOUBLE) {
    return TDBLCOMPLEX;
  }
  return TCOMPLEX;
}

} // namespace Internal

/**
 * @see https://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/c_user/node52.html
 */
int recordTypecode(fitsfile *fptr, const std::string &keyword) {
  int status = 0;
  char value[FLEN_VALUE];
  auto nonconstKeyword = keyword;
  fits_read_keyword(fptr, &keyword[0], value, nullptr, &status);
  mayThrowCfitsioError(status, "Cannot read record: " + keyword);
  char dtype = ' ';
  fits_get_keytype(value, &dtype, &status);
  mayThrowCfitsioError(status, "Cannot deduce type code of keyword: " + keyword);
  // 'C', 'L', 'I', 'F' or 'X', for character string, logical, integer, floating point, or complex
  switch (dtype) {
    case 'C':
      return TSTRING;
    case 'L':
      return TLOGICAL;
    case 'I':
      return Internal::intRecordTypecodeImpl(value);
    case 'F':
      return Internal::floatRecordTypecodeImpl(value);
    case 'X':
      return Internal::complexRecordTypecodeImpl(value);
    default:
      throw CfitsioError(status);
  }
}

} // namespace Header
} // namespace Cfitsio
} // namespace Euclid
