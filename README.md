# Project overview

![EleFits logo](doc/diagrams/out/elefits_square.svg)

## Purpose and scope

EleFits (Euclid libraries and executables for Fits) is a modern C++ package to read and write Fits files which focuses on safety, user-friendliness, and performance.
EleFits provides the only C++ Fits library which features a fully consistent and optimal internal type management system.
Nevertheless, the API is templated and agnostic of the underlying architecture, which makes usage straightforward.
EleFits is compact and involves as few parameters as possible, which makes it both simple and robust.
As compared to CFitsIO, the benefits are mainly those of C++ over C:

* No raw pointers for a safe memory management;
* No type codes but template traits classes which handle architecture specifics;
* No error codes but a proper exception handling;
* Structures dedicated to data storage;
* A lightweight class hierarchy to represent the actual Fits organization (e.g. HDUs, records, columns).

Furthermore, exclusive features are provided, like HDU selectors and automatic buffering of binary tables.

To maximize performance, EleFits is built as a CFitsIO wrapper as thin as possible.
While the two libraries are generally equivalent, optimizations implemented internally make EleFits even faster in some classical cases, unless the CFitsIO user spends considerable development efforts.

## Example usages

The EleFits API was specifically designed to be very fluent and compact.
The following (a bit extreme) example shows how natural it is
to read a column named "RA" in the 4th extension of a Multi-Extension Fits (MEF) file:

```cpp
// Given:
// - string filename: The file name

MefFile f(filename, FileMode::READ);
auto ra = f.access<BintableHdu>(4).columns().read<double>("RA");
```

A more realistic example is creating a Single Image Fits (SIF) file with a keyword record and an array:

```cpp
// Given:
// - string filename: The file name
// - string keyword: The keyword record name
// - int value: The keyword record value
// - string comment: The keyword record comment
// - long width, height: The image size
// - float[width * height] data: The image values

SifFile f(filename, FileMode::CREATE);
f.header().write(keyword, value, "", comment);
f.raster().write(makeRaster(data, width, height));

// For comparison, here is the same use case implemented with CCfits:

long shape[] = { width, height };
auto fits = std::make_unique<FITS>(filename, SHORT_IMG, 2, shape);
auto& primary = fits->pHDU();
primary.addKey(name, value, comment);
primary.write(1, width * height, data);

// And with CFitsIO:

long shape[] = { width, height };
int status = 0;
fitsfile* fptr = nullptr;
fits_create_file(&fptr, filename.c_str(), &status);
fits_create_img(fptr, SHORT_IMG, 2, shape, &status);
fits_write_key(fptr, TDOUBLE, name.c_str(), &value, comment.c_str(), &status);
fits_write_img(fptr, TSHORT, 1, width * height, data, &status);
```

In addition, exclusive features are provided to simplify the implementation of classical use cases.
A few examples are given below.

Files are iterable, and selectors enable looping over filtered HDUs
(here, newly created image HDUs):

```cpp
// Given:
// - MefFile f: The MEF file handler
// - processNewImage: The user-defined function

for (const auto& hdu : f.select<ImageHdu>(HduCategory::Created)) {
  processNewImage(hdu);
}
```

Heterogeneous collections of keyword records can be parsed and written,
and a comprehensive type conversion system is provided:

```cpp
// Given:
// - Header header: The header unit handler

auto records = header.parseAll(KeywordCategory::Reserved);
auto instrument = records.as<std::string>("INSTRUME");
auto exptime = records.as<double>("EXPTIME");
```

Images and tables can be read and written region-wise
by mapping -- possibly non-contiguous -- in-file and in-memory regions, e.g.:

```cpp
// Given:
// - ImageRaster raster: The image data unit handler
// - Raster data: The image container

Region<2> inFile { { 32, 16 }, { 64, 32 } };
Position<2> inMemory { 8, 8 };
raster.readRegionTo({ inFile, inMemory }, data);
```

For binary tables, multiple columns can be read or written at once
to take advantage of an internal buffer:

```cpp
// Given:
// - BintableColumns columns: The binary table data unit handler
// - Column columnA, columnB, columnC: Column containers of various value types

columns.writeSeq(columnA, columnB, columnC);
```

## Licence

LGPL v3

## Tutorial

The tutorial composed of an example program and [associated documentation](https://cnes.github.io/EleFits/4.0.1/tuto.html) is probably the good entry point for newcomers.

## Installation instructions

See [the dedicated page](doc/INSTALL.md).

## User documentation

The [Modules page](https://cnes.github.io/EleFits/4.0.1/modules.html) is the main entry point for usage documentation.
Each so-called documentation module addresses a specific topic to learn how to use EleFits and understand why it is designed the way it is.
The API documentation of related namespaces, classes and functions is linked at the bottom of each module page.

To go further, many other topics are discussed in the [Related pages](https://cnes.github.io/EleFits/4.0.1/pages.html), as unordered documents.
Among others, you'll find there thoughts on CFitsIO, the tutorial, and some design documentation. 

## Feedbacks

Bug reports, reviews, change requests and other feedbacks are welcome!
Feel free to [open issues](https://github.com/CNES/EleFits/issues/new/choose) ;)
