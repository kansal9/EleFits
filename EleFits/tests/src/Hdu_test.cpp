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

#include "EleFitsData/TestRecord.h"
#include "EleFits/FitsFileFixture.h"
#include "EleFits/Hdu.h"
#include "EleFits/MefFile.h"
#include "EleFits/SifFile.h"
#include "ElementsKernel/Temporary.h"

#include <boost/test/unit_test.hpp>

using namespace Euclid::FitsIO;

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Hdu_test)

//-----------------------------------------------------------------------------

template <typename T>
void checkRecordWithFallbackIsReadBack(const Hdu& h, const std::string& keyword) {
  BOOST_TEST(not h.hasKeyword(keyword));
  BOOST_CHECK_THROW(h.parseRecord<T>(keyword), std::exception);
  const Record<T> fallback { keyword, Test::generateRandomValue<T>(), "", "FALLBACK" };
  auto output = h.parseRecordOr<T>(fallback);
  BOOST_TEST((output == fallback));
  const Record<T> input { keyword, Test::generateRandomValue<T>(), "", "INPUT" };
  h.writeRecord(input);
  BOOST_TEST((input != fallback)); // At least the comments differ
  output = h.parseRecordOr<T>(fallback);
  BOOST_TEST(output.keyword == input.keyword);
  BOOST_TEST(Test::approx(output.value, input.value));
  BOOST_TEST(output.unit == input.unit);
  BOOST_TEST(output.comment == input.comment);
}

template <>
void checkRecordWithFallbackIsReadBack<unsigned long>(const Hdu& h, const std::string& keyword) {
  // Wait for CFitsIO bug to be fixed
  (void)h;
  (void)keyword;
}

#define RECORD_WITH_FALLBACK_IS_READ_BACK_TEST(type, name) \
  BOOST_FIXTURE_TEST_CASE(name##_record_with_fallback_is_read_back_test, Test::TemporarySifFile) { \
    checkRecordWithFallbackIsReadBack<type>(this->header(), std::string(#name).substr(0, 8)); \
  }

ELEFITS_FOREACH_RECORD_TYPE(RECORD_WITH_FALLBACK_IS_READ_BACK_TEST)

BOOST_FIXTURE_TEST_CASE(records_with_fallback_are_read_back_test, Test::TemporarySifFile) {
  Record<short> written("SHORT", 1);
  Record<long> fallback("LONG", 10);
  const auto& header = this->header();
  BOOST_TEST(not header.hasKeyword(written.keyword));
  BOOST_TEST(not header.hasKeyword(fallback.keyword));
  header.writeRecord(written);
  written.value++;
  fallback.value++;
  const auto output = header.parseRecordsOr(written, fallback);
  BOOST_TEST(std::get<0>(output).value == written.value - 1);
  BOOST_TEST(std::get<1>(output).value == fallback.value);
}

BOOST_FIXTURE_TEST_CASE(long_string_value_is_read_back_test, Test::TemporarySifFile) {
  const auto& h = this->header();
  const std::string shortStr = "S";
  const std::string longStr =
      "This is probably one of the longest strings "
      "that I have ever written in a serious code.";
  BOOST_TEST(longStr.length() > FLEN_VALUE);
  h.writeRecord("SHORT", shortStr);
  BOOST_TEST(not h.hasKeyword("LONGSTRN"));
  h.writeRecord("LONG", longStr);
  const auto output = h.parseRecord<std::string>("LONG");
  h.parseRecord<std::string>("LONGSTRN");
  BOOST_TEST(output.value == longStr);
  BOOST_TEST(output.hasLongStringValue());
}

void checkHierarchKeywordIsReadBack(const Hdu& h, const std::string& keyword) {
  BOOST_TEST(h.readHeader(false).find("HIERARCH") == std::string::npos); // Not found
  const Record<int> record(keyword, 10);
  BOOST_TEST(record.hasLongKeyword() == (keyword.length() > 8));
  h.writeRecord(record);
  BOOST_TEST(h.readHeader(false).find("HIERARCH") != std::string::npos); // Found
  const auto output = h.parseRecord<int>(keyword);
  BOOST_TEST(output.value == 10);
}

BOOST_FIXTURE_TEST_CASE(long_keyword_is_read_back_test, Test::TemporarySifFile) {
  checkHierarchKeywordIsReadBack(this->header(), "123456789");
}

BOOST_FIXTURE_TEST_CASE(keyword_with_space_is_read_back_test, Test::TemporarySifFile) {
  checkHierarchKeywordIsReadBack(this->header(), "A B");
}

BOOST_FIXTURE_TEST_CASE(keyword_with_symbol_is_read_back_test, Test::TemporarySifFile) {
  checkHierarchKeywordIsReadBack(this->header(), "1$");
}

BOOST_FIXTURE_TEST_CASE(hdu_is_renamed_test, Test::TemporaryMefFile) {
  const auto& h = this->initRecordExt("A");
  BOOST_TEST(h.index() == 1);
  BOOST_TEST(h.readName() == "A");
  h.updateName("B");
  BOOST_TEST(h.readName() == "B");
  h.deleteRecord("EXTNAME");
  BOOST_TEST(h.readName() == "");
}

BOOST_FIXTURE_TEST_CASE(c_str_record_is_read_back_as_string_record_test, Test::TemporarySifFile) {
  const auto& h = this->header();
  h.writeRecord("C_STR", "1");
  const auto output1 = h.parseRecord<std::string>("C_STR");
  BOOST_TEST(output1.value == "1");
  h.updateRecord("C_STR", "2");
  const auto output2 = h.parseRecord<std::string>("C_STR");
  BOOST_TEST(output2.value == "2");
}

BOOST_FIXTURE_TEST_CASE(record_tuple_is_updated_and_read_back_test, Test::TemporarySifFile) {
  const auto& h = this->header();
  const Record<short> short_record { "SHORT", 1 };
  const Record<long> long_record { "LONG", 1000 };
  auto records = std::make_tuple(short_record, long_record);
  h.writeRecords(records);
  BOOST_TEST(h.parseRecord<short>("SHORT") == 1);
  BOOST_TEST(h.parseRecord<long>("LONG") == 1000);
  std::get<0>(records).value = 2;
  std::get<1>(records).value = 2000;
  h.updateRecords(records);
  BOOST_TEST(h.parseRecord<short>("SHORT") == 2);
  BOOST_TEST(h.parseRecord<long>("LONG") == 2000);
}

BOOST_FIXTURE_TEST_CASE(vector_of_any_records_is_read_back_test, Test::TemporarySifFile) {
  const auto& h = this->header();
  std::vector<Record<VariantValue>> records;
  records.push_back({ "STRING", std::string("WIDE") });
  records.push_back({ "FLOAT", 3.14F });
  records.push_back({ "INT", 666 });
  h.writeRecords(records);
  auto parsed = h.parseAllRecords<VariantValue>();
  BOOST_TEST(parsed.as<std::string>("STRING").value == "WIDE");
  BOOST_TEST(parsed.as<int>("INT").value == 666);
  BOOST_CHECK_THROW(parsed.as<std::string>("INT"), std::exception);
}

BOOST_FIXTURE_TEST_CASE(subset_of_vector_of_any_records_is_read_back_test, Test::TemporarySifFile) {
  const auto& h = this->header();
  RecordSeq records(3);
  records.vector[0].assign(Record<std::string>("STRING", "WIDE"));
  records.vector[1].assign(Record<float>("FLOAT", 3.14F));
  records.vector[2].assign(Record<int>("INT", 666));
  h.writeRecords(records, { "FLOAT", "INT" });
  BOOST_CHECK_THROW(h.parseRecord<VariantValue>("STRING"), std::exception);
  auto parsed = h.parseRecordSeq({ "INT" });
  BOOST_TEST(parsed.as<int>("INT").value == 666);
  BOOST_CHECK_THROW(parsed.as<float>("FLOAT"), std::exception);
}

BOOST_FIXTURE_TEST_CASE(brackets_in_comment_are_read_back_test, Test::TemporaryMefFile) {
  const auto& primary = this->accessPrimary<>();
  primary.writeRecord("PLAN_ID", 1, "", "[0:1] SOC Planning ID");
  const auto intRecord = primary.parseRecord<int>("PLAN_ID");
  BOOST_TEST(intRecord.unit == "0:1");
  BOOST_TEST(intRecord.comment == "SOC Planning ID");
  primary.writeRecord("STRING", std::string("1"), "", "[0:1] SOC Planning ID");
  const auto stringRecord = primary.parseRecord<std::string>("STRING");
  BOOST_TEST(stringRecord.unit == "0:1");
  BOOST_TEST(stringRecord.comment == "SOC Planning ID");
  primary.writeRecord("CSTR", "1", "", "[0:1] SOC Planning ID");
  const auto cstrRecord = primary.parseRecord<std::string>("CSTR");
  BOOST_TEST(cstrRecord.unit == "0:1");
  BOOST_TEST(cstrRecord.comment == "SOC Planning ID");
  primary.writeRecord("WEIRD", 2, "m", "[0:1] SOC Planning ID");
  const auto weirdRecord = primary.parseRecord<std::string>("WEIRD");
  BOOST_TEST(weirdRecord.unit == "m");
  BOOST_TEST(weirdRecord.comment == "[0:1] SOC Planning ID");
}

BOOST_FIXTURE_TEST_CASE(comment_and_history_are_written, Test::TemporarySifFile) {
  const auto& header = this->header();
  const std::string comment = "BLUE";
  const std::string history = "BEAVER";
  header.writeComment(comment);
  header.writeHistory(history);
  const auto contents = header.readHeader();
  BOOST_TEST((contents.find(comment) != std::string::npos));
  BOOST_TEST((contents.find(history) != std::string::npos));
}

BOOST_FIXTURE_TEST_CASE(full_header_is_read_as_string_test, Test::TemporarySifFile) {
  const auto header = this->header().readHeader(); // TODO check with false
  BOOST_TEST(header.size() > 0);
  BOOST_TEST(header.size() % 80 == 0);
  // TODO check contents
}

BOOST_FIXTURE_TEST_CASE(records_are_read_as_a_struct_test, Test::TemporarySifFile) {
  struct MyHeader {
    bool b;
    int i;
    float f;
    std::string s;
  };
  const auto& header = this->header();
  const MyHeader input { false, 1, 3.14F, "VAL" };
  header.writeRecords(
      Record<bool>("BOOL", input.b),
      Record<int>("INT", input.i),
      Record<float>("FLOAT", input.f),
      Record<std::string>("STRING", input.s));
  const auto output = header.parseRecordsAs<MyHeader>(
      Named<bool>("BOOL"),
      Named<int>("INT"),
      Named<float>("FLOAT"),
      Named<std::string>("STRING"));
  BOOST_TEST(output.b == input.b);
  BOOST_TEST(output.i == input.i);
  BOOST_TEST(output.f == input.f);
  BOOST_TEST(output.s == input.s);
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()
