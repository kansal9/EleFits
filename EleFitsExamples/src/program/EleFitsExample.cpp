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

#include "EleCfitsioWrapper/CfitsioFixture.h"
#include "EleFitsData/TestColumn.h"
#include "EleFitsData/TestRaster.h"
#include "EleFits/MefFile.h"
#include "EleFitsUtils/ProgramOptions.h"
#include "ElementsKernel/ProgramHeaders.h"

#include <boost/program_options.hpp>
#include <map>
#include <string>

using boost::program_options::value;

using namespace Euclid;
using namespace Fits;

class EleFits_Example : public Elements::Program {

public:
  std::pair<OptionsDescription, PositionalOptionsDescription> defineProgramArguments() override {
    Euclid::Fits::ProgramOptions options;
    options.positional("output", value<std::string>()->default_value("/tmp/test.fits"), "Output file");
    return options.asPair();
  }

  Elements::ExitCode mainMethod(std::map<std::string, VariableValue>& args) override {

    Elements::Logging logger = Elements::Logging::getLogger("EleFits_Example");

    const std::string filename = args["output"].as<std::string>();

    logger.info();
    {
      logger.info() << "Creating Fits file: " << filename;
      //! [Create Fits]
      MefFile f(filename, FileMode::Overwrite);
      //! [Create Fits]
      const auto& primary = f.accessPrimary<>(); // We don't need to specify the HDU type for metadata work
      logger.info() << "Writing new record: VALUE = 1";
      //! [Write record]
      primary.header().write("VALUE", 1);
      //! [Write record]
      logger.info() << "Updating record: VALUE = 2";
      //! [Update record]
      primary.header().write("VALUE", 2);
      //! [Update record]

      logger.info();

      Test::SmallTable table; // Predefined table for testing purpose
      logger.info() << "Creating binary table extension: SMALLTBL";
      //! [Create binary table ext]
      f.assignBintableExt("SMALLTBL", table.numCol, table.radecCol, table.nameCol, table.distMagCol);
      //! [Create binary table ext]

      logger.info();

      Test::SmallRaster raster; // Predefined image raster for testing purpose
      logger.info() << "Creating image extension: SMALLIMG";
      //! [Create image ext]
      const auto& ext = f.assignImageExt("SMALLIMG", raster);
      //! [Create image ext]
      logger.info() << "Writing record: STRING = string";
      Record<std::string> strRecord("STRING", "string");
      logger.info() << "Writing record: INTEGER = 8";
      Record<int> intRecord("INTEGER", 8);
      ext.header().writeSeq(strRecord, intRecord);

      logger.info();

      logger.info() << "Closing file.";
      //! [Close Fits]
      f.close(); // We close the file manually for demo purpose, but this is done by the destructor otherwise
      //! [Close Fits]
    }
    logger.info();
    {

      logger.info() << "Reopening file.";
      //! [Open Fits]
      MefFile f(filename, FileMode::Read);
      //! [Open Fits]
      //! [Read record]
      const auto recordValue = f.primary().header().parse<int>("VALUE");
      //! [Read record]
      logger.info() << "Reading record: VALUE = " << recordValue;

      logger.info();

      logger.info() << "Reading binary table.";
      //! [Find HDU by name]
      const auto& bintableExt = f.accessFirst<BintableHdu>("SMALLTBL");
      //! [Find HDU by name]
      //! [Get HDU index]
      const auto index = bintableExt.index();
      //! [Get HDU index]
      logger.info() << "HDU index: " << index;
      //! [Read column]
      const auto ids = bintableExt.columns().read<int>("ID").vector();
      const auto firstCell = ids[0];
      //! [Read column]
      logger.info() << "First id: " << firstCell;
      const auto names = bintableExt.columns().read<std::string>("NAME").vector();
      logger.info() << "Last name: " << names[names.size() - 1];

      logger.info();

      logger.info() << "Reading image.";
      //! [Find HDU by index]
      const auto& ext2 = f.access<>(2);
      //! [Find HDU by index]
      //! [Get HDU name]
      const auto extname = ext2.readName();
      //! [Get HDU name]
      logger.info() << "Name of HDU #3: " << extname;
      const auto records = ext2.header().parseSeq(Named<std::string>("STRING"), Named<int>("INTEGER"));
      logger.info() << "Reading record: STRING = " << std::get<0>(records).value;
      logger.info() << "Reading record: INTEGER = " << std::get<1>(records).value;
      const auto& imageExt = f.accessFirst<ImageHdu>("SMALLIMG");
      //! [Read raster]
      const auto image = imageExt.readRaster<float>();
      const auto firstPixel = image[{ 0, 0 }];
      const auto lastPixel = image.at({ -1, -1 }); // at() allows backward indexing
      //! [Read raster]
      logger.info() << "First pixel: " << firstPixel;
      logger.info() << "Last pixel: " << lastPixel;

      logger.info();

      logger.info() << "File will be closed at execution end.";

      logger.info();

      return Elements::ExitCode::OK;
    } // File is closed by destructor
  }
};

MAIN_FOR(EleFits_Example)
