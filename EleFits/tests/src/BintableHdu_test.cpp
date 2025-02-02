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

#include "EleFitsData/TestColumn.h"
#include "EleFits/BintableHdu.h"
#include "EleFits/FitsFileFixture.h"
#include "EleFits/MefFile.h"
#include "ElementsKernel/Temporary.h"

#include <boost/test/unit_test.hpp>

using namespace Euclid::Fits;

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(BintableHdu_test)

//-----------------------------------------------------------------------------

template <typename T>
void checkScalar() {
  Test::RandomScalarColumn<T> input;
  const std::string filename = Elements::TempFile().path().string();
  MefFile file(filename, FileMode::Temporary);
  file.assignBintableExt("BINEXT", input);
  const auto output = file.accessFirst<BintableHdu>("BINEXT").readColumn<T>(input.info().name);
  BOOST_TEST(output.vector() == input.vector());
}

template <typename T>
void checkVector() {
  constexpr long rowCount = 10;
  constexpr long repeatCount = 2;
  Test::RandomScalarColumn<T> input(rowCount * repeatCount);
  input.reshape(repeatCount);
  const std::string filename = Elements::TempFile().path().string();
  MefFile file(filename, FileMode::Temporary);
  file.initBintableExt("BINEXT", input.info());
  file.accessFirst<BintableHdu>("BINEXT").writeColumn(input);
  const auto output = file.accessFirst<BintableHdu>("BINEXT").readColumn<T>(input.info().name);
}

/**
 * We test only one type here to check the flow from the top-level API to CFitsIO.
 * Support for other types is tested in EleCfitsioWrapper.
 */
BOOST_AUTO_TEST_CASE(float_test) {
  checkScalar<float>();
  checkVector<float>();
}

BOOST_AUTO_TEST_CASE(empty_column_test) {
  const std::string filename = Elements::TempFile().path().string();
  VecColumn<float> input({ "NAME", "", 1 }, std::vector<float>());
  MefFile file(filename, FileMode::Temporary);
  file.assignBintableExt("BINEXT", input);
}

BOOST_AUTO_TEST_CASE(colsize_mismatch_test) {
  VecColumn<float> input0({ "COL0", "", 1 }, std::vector<float>());
  Test::RandomScalarColumn<float> input1(1);
  Test::RandomScalarColumn<float> input2(2);
  input1.rename("COL1");
  input2.rename("COL2");
  const std::string filename = Elements::TempFile().path().string();
  MefFile file(filename, FileMode::Temporary);
  BOOST_CHECK_NO_THROW(file.assignBintableExt("0AND1", input0, input1));
  BOOST_CHECK_NO_THROW(file.assignBintableExt("1AND0", input1, input0));
  BOOST_CHECK_NO_THROW(file.assignBintableExt("1AND2", input1, input2));
  BOOST_CHECK_NO_THROW(file.assignBintableExt("2AND1", input2, input1));
}

BOOST_FIXTURE_TEST_CASE(counting_test, Test::TemporaryMefFile) {
  const std::string name1 = "COL1";
  Test::RandomScalarColumn<std::string> column1;
  column1.rename(name1);
  const std::string name2 = "COL2";
  Test::RandomScalarColumn<double> column2;
  column2.rename(name2);
  const auto& ext = assignBintableExt("", column1, column2);
  const auto& du = ext.columns();
  BOOST_TEST(du.readColumnCount() == 2);
  BOOST_TEST(du.readRowCount() == column1.rowCount());
  BOOST_TEST(du.has(name1));
  BOOST_TEST(du.has(name2));
  BOOST_TEST(not du.has("NOTHERE"));
}

BOOST_FIXTURE_TEST_CASE(multi_column_test, Test::TemporaryMefFile) {
  const auto intColumn = Test::RandomTable::generateColumn<int>("INT");
  const auto floatColumn = Test::RandomTable::generateColumn<float>("FLOAT");
  const auto& ext = assignBintableExt("", intColumn, floatColumn);
  const auto& du = ext.columns();
  const auto byName = du.readSeq(Named<int>(intColumn.info().name), Named<float>(floatColumn.info().name));
  BOOST_TEST(std::get<0>(byName).vector() == intColumn.vector());
  BOOST_TEST(std::get<1>(byName).vector() == floatColumn.vector());
  const auto byIndex = du.readSeq(Indexed<int>(0), Indexed<float>(1));
  BOOST_TEST(std::get<0>(byIndex).vector() == intColumn.vector());
  BOOST_TEST(std::get<1>(byIndex).vector() == floatColumn.vector());
}

BOOST_FIXTURE_TEST_CASE(column_renaming_test, Test::TemporaryMefFile) {
  std::vector<ColumnInfo<int>> header { { "A" }, { "B" }, { "C" } };
  const auto& ext = initBintableExt("TABLE", header[0], header[1], header[2]);
  const auto& du = ext.columns();
  auto names = du.readAllNames();
  for (std::size_t i = 0; i < header.size(); ++i) {
    BOOST_TEST(du.readName(i) == header[i].name);
    BOOST_TEST(names[i] == header[i].name);
  }
  header[0].name = "A2";
  header[2].name = "C2";
  du.rename(0, header[0].name);
  du.rename("C", header[2].name);
  names = du.readAllNames();
  for (std::size_t i = 0; i < header.size(); ++i) {
    BOOST_TEST(du.readName(i) == header[i].name);
    BOOST_TEST(names[i] == header[i].name);
  }
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()
