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

#if defined(_ELECFITSIOWRAPPER_IMAGEWRAPPER_IMPL) || defined(CHECK_QUALITY)

  #include "EleCfitsioWrapper/ImageWrapper.h"

namespace Euclid {
namespace Cfitsio {
namespace ImageIo {

/**
 * @brief Variable dimension case.
 */
template <>
Fits::Position<-1> readShape<-1>(fitsfile* fptr);

template <long n = 2>
Fits::Position<n> readShape(fitsfile* fptr) {
  Fits::Position<n> shape;
  int status = 0;
  fits_get_img_size(fptr, n, &shape[0], &status);
  CfitsioError::mayThrow(status, fptr, "Cannot read raster shape.");
  return shape;
}

template <long n>
void updateShape(fitsfile* fptr, const Fits::Position<n>& shape) {
  int status = 0;
  int imgtype = 0;
  fits_get_img_type(fptr, &imgtype, &status);
  auto nonconstShape = shape;
  fits_resize_img(fptr, imgtype, shape.size(), nonconstShape.data(), &status);
  CfitsioError::mayThrow(status, fptr, "Cannot reshape raster.");
}

template <typename T, long n>
void updateTypeShape(fitsfile* fptr, const Fits::Position<n>& shape) {
  int status = 0;
  auto nonconstShape = shape;
  fits_resize_img(fptr, TypeCode<T>::bitpix(), shape.size(), nonconstShape.data(), &status);
  CfitsioError::mayThrow(status, fptr, "Cannot reshape raster.");
}

template <typename T, long n>
Fits::VecRaster<T, n> readRaster(fitsfile* fptr) {
  Fits::VecRaster<T, n> raster(readShape<n>(fptr));
  readRasterTo(fptr, raster);
  return raster;
}

template <typename T, long n>
void readRasterTo(fitsfile* fptr, Fits::Raster<T, n>& destination) {
  int status = 0;
  const auto size = destination.size();
  fits_read_img(
      fptr,
      TypeCode<T>::forImage(),
      1, // Number 1 is a 1-based index (so we read the whole raster here)
      size,
      nullptr,
      destination.data(),
      nullptr,
      &status);
  CfitsioError::mayThrow(status, fptr, "Cannot read raster.");
}

template <typename T, long n>
void readRasterTo(fitsfile* fptr, Fits::Subraster<T, n>& destination) {
  const auto region = Fits::Region<n>::fromShape(Fits::Position<n>::zero(), readShape<n>(fptr));
  readRegionTo(fptr, region, destination);
}

template <typename T, long m, long n>
Fits::VecRaster<T, m> readRegion(fitsfile* fptr, const Fits::Region<n>& region) {
  Fits::VecRaster<T, m> raster(region.shape().template slice<m>());
  readRegionTo(fptr, region, raster);
  return raster;
}

template <typename T, long m, long n>
void readRegionTo(fitsfile* fptr, const Fits::Region<n>& region, Fits::Raster<T, m>& raster) {
  int status = 0;
  const std::size_t dim = region.dimension();
  Fits::Position<n> front = region.front; // Copy for const-correctness
  Fits::Position<n> back = region.back; // idem
  std::vector<long> step(dim, 1);
  for (std::size_t i = 0; i < dim; ++i) {
    front[i]++; // CFitsIO is 1-based
    back[i]++; // idem
  }
  fits_read_subset(
      fptr,
      TypeCode<T>::forImage(),
      front.data(),
      back.data(),
      step.data(),
      nullptr,
      raster.data(),
      nullptr,
      &status);
  CfitsioError::mayThrow(status, fptr, "Cannot read image region.");
}

template <typename T, long m, long n>
void readRegionTo(fitsfile* fptr, const Fits::Region<n>& region, Fits::Subraster<T, m>& destination) {

  /* 1-based, flatten region (beginning of each line) */
  Fits::Region<n> srcRegion = region + 1;
  srcRegion.back[0] = srcRegion.front[0];
  const auto srcCount = srcRegion.size();

  /* Screening positions */
  auto srcFront = srcRegion.front;
  auto srcBack = srcFront;
  srcBack[0] += region.shape()[0] - 1;
  auto dstFront = destination.region().front;
  Fits::RegionScreener<n> srcScreener(srcRegion, { srcBack, dstFront });

  /* Step */
  std::vector<long> step(srcRegion.dimension(), 1L);

  /* Process each line */
  int status = 0;
  for (long i = 0; i < srcCount; ++i) {
    fits_read_subset(
        fptr,
        TypeCode<T>::forImage(),
        srcFront.data(),
        srcBack.data(),
        step.data(),
        nullptr,
        &destination.parent()[dstFront],
        nullptr,
        &status);
    CfitsioError::mayThrow(status, fptr, "Cannot read image region.");
    srcScreener.next();
    srcFront = srcScreener.current();
    srcBack = srcScreener.followers()[0];
    dstFront = srcScreener.followers()[1];
  }
}

template <typename T, long n>
void writeRaster(fitsfile* fptr, const Fits::Raster<T, n>& raster) {
  mayThrowReadonlyError(fptr);
  int status = 0;
  const auto begin = raster.data();
  const auto end = begin + raster.size();
  std::vector<std::decay_t<T>> nonconstData(begin, end); // For const-correctness issue
  fits_write_img(fptr, TypeCode<T>::forImage(), 1, raster.size(), nonconstData.data(), &status);
  CfitsioError::mayThrow(status, fptr, "Cannot write image.");
}

template <typename T, long m, long n>
void writeRegion(fitsfile* fptr, const Fits::Raster<T, m>& raster, const Fits::Position<n>& destination) {
  int status = 0;
  auto front = destination + 1;
  const auto shape = raster.shape().extend(destination);
  auto back = destination + shape; // = front + raster.shape() - 1
  const auto begin = raster.data();
  const auto end = begin + raster.size();
  std::vector<std::decay_t<T>> nonconstData(begin, end); // For const-correctness issue
  fits_write_subset(fptr, TypeCode<T>::forImage(), front.data(), back.data(), nonconstData.data(), &status);
  CfitsioError::mayThrow(status, fptr, "Cannot write image region.");
}

template <typename T, long m, long n>
void writeRegion(fitsfile* fptr, const Fits::Subraster<T, m>& subraster, const Fits::Position<n>& destination) {

  /* 1-based, flatten region (beginning of each line) */
  const auto shape = subraster.shape().extend(destination);
  Fits::Region<n> dstRegion { destination + 1, destination + shape };
  dstRegion.back[0] = dstRegion.front[0];

  /* Screening positions */
  const auto dstSize = shape[0];
  const auto delta = subraster.region().front.extend(dstRegion.front) - dstRegion.front;

  /* Process each line */
  int status = 0;
  Fits::Position<n> dstBack;
  Fits::Position<n> srcFront;
  std::vector<std::decay_t<T>> line(dstSize);
  for (auto dstFront : dstRegion) {
    dstBack = dstFront;
    dstBack[0] += dstSize - 1;
    srcFront = dstFront + delta;
    line.assign(&subraster[srcFront], &subraster[srcFront] + dstSize);
    fits_write_pix(fptr, TypeCode<T>::forImage(), dstFront.data(), dstSize, line.data(), &status);
    // fits_write_subset(fptr, TypeCode<T>::forImage(), dstFront.data(), dstBack.data(), line.data(), &status);
    CfitsioError::mayThrow(status, fptr, "Cannot write image region.");
  }
}

} // namespace ImageIo
} // namespace Cfitsio
} // namespace Euclid

#endif
