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

#if defined(_ELEFITS_BINTABLECOLUMNS_IMPL) || defined(CHECK_QUALITY)

#include "EleCfitsioWrapper/BintableWrapper.h"
#include "EleCfitsioWrapper/HeaderWrapper.h" // TODO rm when implementation of init(Seq) is in BintableWrapper
#include "EleFits/BintableColumns.h"

namespace Euclid {
namespace Fits {

// Implementation rules for overloads
//
// - Flow should go from names to indices: never call readName() internally, and call readIndex() once;
// - Variadic methods should call TSeq methods through std::forward_as_tuple because TSec is more generic;
// - TSeq&& should be forwarded as std::forward<TSeq>();
// - Duplication should be minimal: when there are two ways with unavoidable duplication, choose the minimalist option.
//
// Exceptions to these rules must be explicitely justified.

// readInfo

template <typename T>
ColumnInfo<T> BintableColumns::readInfo(ColumnKey key) const {
  return Cfitsio::BintableIo::readColumnInfo<T>(m_fptr, key.index(*this) + 1); // 1-based
}

// read

template <typename T>
VecColumn<T> BintableColumns::read(ColumnKey key) const {
  return readSegment<T>({0, -1}, key); // FIXME useless readRowCount()
}

// readTo

template <typename T>
void BintableColumns::readTo(Column<T>& column) const {
  readTo<T>(column.info().name, column);
}

template <typename T>
void BintableColumns::readTo(ColumnKey key, Column<T>& column) const {
  readSegmentTo<T>({0, -1}, key, column);
}

// readSegment

template <typename T>
VecColumn<T> BintableColumns::readSegment(const Segment& rows, ColumnKey key) const {
  const auto index = key.index(*this);
  auto resolvedRows = rows;
  if (rows.back == -1) {
    resolvedRows.back = readRowCount() - 1;
  }
  VecColumn<T> column(readInfo<T>(std::move(key)), resolvedRows.size());
  readSegmentTo<T>(resolvedRows, index, column);
  return column;
}

// readSegmentTo

template <typename T>
void BintableColumns::readSegmentTo(FileMemSegments rows, Column<T>& column) const {
  readSegmentTo<T>(std::move(rows), column.info().name, column);
}

template <typename T>
void BintableColumns::readSegmentTo(FileMemSegments rows, ColumnKey key, Column<T>& column) const {
  m_touch();
  rows.resolve(readRowCount() - 1, column.rowCount() - 1);
  auto slice = column.slice(rows.memory()); // TODO do we need a temporary variable?
  Cfitsio::BintableIo::readColumnSegment<T>(
      m_fptr,
      Segment {rows.file().front + 1, rows.file().back + 1}, // TODO operator+
      key.index(*this) + 1, // 1-based
      slice);
}

// readSeq

template <typename TKey, typename... Ts>
std::tuple<VecColumn<Ts>...> BintableColumns::readSeq(const TypedKey<Ts, TKey>&... keys) const {
  const auto rowCount = readRowCount();
  std::tuple<VecColumn<Ts>...> res {VecColumn<Ts>(readInfo<Ts>(keys.key), rowCount)...};
  readSeqTo({ColumnKey(keys.key)...}, res);
  return res;
}

template <typename T>
std::vector<VecColumn<T>> BintableColumns::readSeq(std::vector<ColumnKey> keys) const {
  const auto rowCount = readRowCount();
  std::vector<VecColumn<T>> res(keys.size());
  std::transform(keys.begin(), keys.end(), res.begin(), [&](ColumnKey& k) {
    return VecColumn<T>(readInfo<T>(k.index(*this)), rowCount);
  });
  readSeqTo(std::move(keys), res);
  return res;
}

// readSeqTo

template <typename TSeq>
void BintableColumns::readSeqTo(TSeq&& columns) const {
  auto keys = seqTransform<std::vector<ColumnKey>>(columns, [&](const auto& c) {
    return c.info().name;
  });
  readSeqTo(std::move(keys), std::forward<TSeq>(columns));
}

template <typename... Ts>
void BintableColumns::readSeqTo(Column<Ts>&... columns) const {
  readSeqTo(std::forward_as_tuple(columns...));
}

template <typename TSeq>
void BintableColumns::readSeqTo(std::vector<ColumnKey> keys, TSeq&& columns) const {
  readSegmentSeqTo(0, std::move(keys), std::forward<TSeq>(columns));
}

template <typename... Ts>
void BintableColumns::readSeqTo(std::vector<ColumnKey> keys, Column<Ts>&... columns) const {
  readSeqTo(std::move(keys), std::forward_as_tuple(columns...));
}

// readSegmentSeq

template <typename TKey, typename... Ts>
std::tuple<VecColumn<Ts>...>
BintableColumns::readSegmentSeq(const Segment& rows, const TypedKey<Ts, TKey>&... keys) const {
  auto resolvedRows = rows;
  if (rows.back == -1) {
    resolvedRows.back = readRowCount() - 1;
  }
  std::tuple<VecColumn<Ts>...> columns {{readInfo<Ts>(keys.key), resolvedRows.size()}...};
  readSegmentSeqTo(resolvedRows, {ColumnKey(keys.key)...}, columns);
  return columns;
}

// readSegmentSeqTo

template <typename TSeq>
void BintableColumns::readSegmentSeqTo(FileMemSegments rows, TSeq&& columns) const {
  auto keys = seqTransform<std::vector<ColumnKey>>(columns, [&](const auto& c) {
    return c.info().name;
  });
  readSegmentSeqTo(std::move(rows), keys, std::forward<TSeq>(columns));
}

template <typename... Ts>
void BintableColumns::readSegmentSeqTo(FileMemSegments rows, Column<Ts>&... columns) const {
  readSegmentSeqTo(std::move(rows), {ColumnKey(columns.info().name)...}, columns...);
  // Could forward_as_tuple but would be 1 more indirection for the same amount of lines
}

template <typename TSeq>
void BintableColumns::readSegmentSeqTo(FileMemSegments rows, std::vector<ColumnKey> keys, TSeq&& columns) const {
  const auto bufferSize = readBufferRowCount();
  const long rowCount = columnsRowCount(std::forward<TSeq>(columns));
  rows.resolve(readRowCount() - 1, rowCount - 1);
  const long lastMemRow = rows.memory().back;
  for (Segment file = Segment::fromSize(rows.file().front, bufferSize), // TODO use a FileMemSegments
       mem = Segment::fromSize(rows.memory().front, bufferSize);
       mem.front <= lastMemRow; // FIXME file += bufferSize, mem += bufferSize) {
       file.front += bufferSize, file.back += bufferSize, mem.front += bufferSize, mem.back += bufferSize) {
    if (mem.back > lastMemRow) {
      mem.back = lastMemRow;
    }
    auto it = keys.begin();
    seqForeach(std::forward<TSeq>(columns), [&](auto& c) {
      readSegmentTo({file.front, mem}, it->index(*this), c);
      ++it;
    });
  }
}

template <typename... Ts>
void BintableColumns::readSegmentSeqTo(FileMemSegments rows, std::vector<ColumnKey> keys, Column<Ts>&... columns)
    const {
  readSegmentSeqTo(std::move(rows), std::move(keys), std::forward_as_tuple(columns...));
}

// write

template <typename T>
void BintableColumns::write(const Column<T>& column) const {
  writeSegment(0, column);
}

// init

template <typename T>
void BintableColumns::init(const ColumnInfo<T>& info, long index) const {
  auto name = Cfitsio::toCharPtr(info.name);
  auto tform = Cfitsio::toCharPtr(Cfitsio::TypeCode<T>::tform(info.repeatCount));
  int status = 0;
  int cfitsioIndex = index == -1 ? Cfitsio::BintableIo::columnCount(m_fptr) + 1 : index + 1;
  fits_insert_col(m_fptr, cfitsioIndex, name.get(), tform.get(), &status);
  Cfitsio::CfitsioError::mayThrow(status, m_fptr, "Cannot init new column: #" + std::to_string(index));
  if (info.unit != "") {
    const Record<std::string> record {"TUNIT" + std::to_string(cfitsioIndex), info.unit, "", "physical unit of field"};
    Cfitsio::HeaderIo::updateRecord(m_fptr, record);
  }
  // TODO to Cfitsio
}

// writeSegment

template <typename T>
void BintableColumns::writeSegment(FileMemSegments rows, const Column<T>& column) const {
  m_edit();
  rows.resolve(readRowCount() - 1, column.rowCount() - 1);
  Cfitsio::BintableIo::writeColumnSegment(m_fptr, rows.file().front + 1, column.slice(rows.memory()));
}

// writeSeq

template <typename TSeq>
void BintableColumns::writeSeq(TSeq&& columns) const {
  writeSegmentSeq(0, std::forward<TSeq>(columns));
}

template <typename... Ts>
void BintableColumns::writeSeq(const Column<Ts>&... columns) const {
  writeSeq(std::forward_as_tuple(columns...));
}

template <typename TSeq>
void BintableColumns::initSeq(long index, TSeq&& infos) const {
  m_edit();
  const auto nameVec = seqTransform<std::vector<std::string>>(infos, [&](const auto& info) {
    return info.name;
  });
  Cfitsio::CStrArray names(nameVec);
  const auto tformVec = seqTransform<std::vector<std::string>>(infos, [&](const auto& info) {
    using Value = typename std::decay_t<decltype(info)>::Value;
    return Cfitsio::TypeCode<std::decay_t<Value>>::tform(info.repeatCount);
  });
  Cfitsio::CStrArray tforms(tformVec);
  int status = 0;
  int cfitsioIndex = index == -1 ? Cfitsio::BintableIo::columnCount(m_fptr) + 1 : index + 1;
  fits_insert_cols(m_fptr, cfitsioIndex, names.size(), names.data(), tforms.data(), &status);
  // TODO to Cfitsio
  long i = cfitsioIndex;
  seqForeach(std::forward<TSeq>(infos), [&](const auto& info) { // FIXME duplication
    if (info.unit != "") {
      const Record<std::string> record {"TUNIT" + std::to_string(i), info.unit, "", "physical unit of field"};
      Cfitsio::HeaderIo::updateRecord(m_fptr, record);
    }
    ++i;
  });
  // TODO to Cfitsio
}

template <typename... Ts>
void BintableColumns::initSeq(long index, const ColumnInfo<Ts>&... infos) const {
  initSeq(index, std::forward_as_tuple(infos...));
}

// writeSegmentSeq

template <typename TSeq>
void BintableColumns::writeSegmentSeq(FileMemSegments rows, TSeq&& columns) const {
  const auto rowCount = columnsRowCount(std::forward<TSeq>(columns));
  rows.resolve(readRowCount() - 1, rowCount - 1);
  const long lastMemRow = rows.memory().back;
  const auto bufferSize = readBufferRowCount();
  for (auto mem = Segment::fromSize(rows.memory().front, bufferSize), // TODO use a FileMemSegments
       file = Segment::fromSize(rows.file().front, bufferSize);
       mem.front <= lastMemRow; // TODO mem += bufferSize, file += bufferSize) {
       mem.front += bufferSize, mem.back += bufferSize, file.front += bufferSize, file.back += bufferSize) {
    if (mem.back > lastMemRow) {
      mem.back = lastMemRow;
    }
    seqForeach(std::forward<TSeq>(columns), [&](const auto& c) {
      writeSegment({file.front, mem}, c); // FIXME don't recalculate index
    });
  }
}

template <typename... Ts>
void BintableColumns::writeSegmentSeq(FileMemSegments rows, const Column<Ts>&... columns) const {
  writeSegmentSeq(std::move(rows), std::forward_as_tuple(columns...));
}

template <typename TSeq>
long columnsRowCount(TSeq&& columns) {
  long rows = -1;
  seqForeach(std::forward<TSeq>(columns), [&](const auto& c) {
    const auto cRows = c.rowCount();
    if (rows == -1) {
      rows = cRows;
    } else if (cRows != rows) {
      throw FitsError("Columns do not have the same number of rows."); // FIXME clean
    }
  });
  return rows;
}

} // namespace Fits
} // namespace Euclid

#endif
