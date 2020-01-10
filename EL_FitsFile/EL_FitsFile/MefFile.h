/**
 * @file EL_FitsFile/MefFile.h
 * @date 08/30/19
 * @author user
 *
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

#ifndef _EL_FITSFILE_MEFFILE_H
#define _EL_FITSFILE_MEFFILE_H

#include "EL_CfitsioWrapper/HduWrapper.h"

#include "EL_FitsFile/FitsFile.h"

namespace Euclid {
namespace FitsIO {

/**
 * @brief Multi-Extension Fits file reader-writer.
 */
class MefFile : public FitsFile {

public:

	using FitsFile::Permission;

	/**
	 * @brief Create a new MefFile with given filename and permission.
	 */
	MefFile(std::string filename, Permission permission);

	virtual ~MefFile() = default;

	/**
	 * @brief Access the Hdu at given index.
	 * @tparam The type of Hdu: ImageHdu, BintableHdu or Hdu to just handle metadata.
	 * @return A reference to the Hdu reader-writer.
	 * @details
	 * The type can be ImageHdu, BintableHdu or unspecified (base class Hdu).
	 * In the latter case, if needs be, the returned Hdu can still be cast to an ImageHdu or BintableHdu,
	 * or merely be used as a metadata reader-writer, e.g.:
	 */
	template<class T=RecordHdu>
	T& access(std::size_t index);

	/**
	 * @brief Access the first Hdu with given name.
	 * @see access
	 */
	template<class T=RecordHdu>
	T& access_first(std::string name);

	/**
	 * @brief Access the Primary Hdu.
	 * @see access
	 */
	template<class T=RecordHdu>
	T& access_primary();

	/**
	 * @brief Append an extension with optional name.
	 */
	template<class T=RecordHdu>
	T& append_ext(T extension);

	/**
	 * @brief Append a new RecordHdu (as empty ImageHdu) with given name.
	 */
	RecordHdu& init_record_ext(std::string name); //TODO
	/**
	 * @brief Append a new ImageHdu with given shape.
	 * @return A reference to the new ImageHdu.
	 */
	template<typename T, std::size_t n>
	ImageHdu& init_image_ext(std::string name, const pos_type<n>& shape);

	/**
	 * @brief Append an ImageHdu with given data.
	 * @see init_image_ext
	 */
	template<typename T, std::size_t n>
	ImageHdu& assign_image_ext(std::string name, const Raster<T, n>& raster);

	/**
	 * @brief Append a BintableHdu with given column info.
	 * @return A reference to the new BintableHdu.
	 */
	template<typename ...Ts>
	BintableHdu& init_bintable_ext(std::string name, const ColumnInfo<Ts>&... header);

	/**
	 * @brief Append a BintableHdu with given data.
	 * @see init_bintable_ext
	 */
	template<typename ...Ts>
	BintableHdu& assign_bintable_ext(std::string name, const Column<Ts>&... header);

protected:

	/**
	 * @brief Vector of RecordHdus (castable to ImageHdu or BintableHdu).
	 * @warning m_hdus is 0-based while Cfitsio HDUs are 1-based.
	 */
	std::vector<std::unique_ptr<RecordHdu>> m_hdus;

};


/////////////////////
// IMPLEMENTATION //
///////////////////


template<class T>
T& MefFile::access(std::size_t index) {
    Cfitsio::Hdu::goto_index(m_fptr, index);
    auto hdu_type = Cfitsio::Hdu::current_type(m_fptr);
	m_hdus.reserve(index);
    auto& ptr = m_hdus[index-1];
    switch (hdu_type) {
    case Cfitsio::Hdu::Type::IMAGE:
        ptr.reset(new ImageHdu(m_fptr, index));
        break;
    case Cfitsio::Hdu::Type::BINTABLE:
        ptr.reset(new BintableHdu(m_fptr, index));
        break;
    default:
		ptr.reset(new RecordHdu(m_fptr, index));
        break;
    }
    return dynamic_cast<T&>(*ptr.get());
}

template<class T>
T& MefFile::access_first(std::string name) {
    Cfitsio::Hdu::goto_name(m_fptr, name);
    return access<T>(Cfitsio::Hdu::current_index(m_fptr));
}

template<class T>
T& MefFile::access_primary() {
	return access<T>(1);
}

template<typename T, std::size_t n>
ImageHdu& MefFile::init_image_ext(std::string name, const pos_type<n>& shape) {
    Cfitsio::Hdu::create_image_extension<T, n>(m_fptr, name, shape);
	const auto size = m_hdus.size();
	m_hdus.push_back(std::unique_ptr<RecordHdu>(new ImageHdu(m_fptr, size+1)));
	return dynamic_cast<ImageHdu&>(*m_hdus[size].get());
}

template<typename T, std::size_t n>
ImageHdu& MefFile::assign_image_ext(std::string name, const Raster<T, n>& raster) {
    Cfitsio::Hdu::create_image_extension(m_fptr, name, raster);
	const auto size = m_hdus.size();
	m_hdus.push_back(std::unique_ptr<RecordHdu>(new ImageHdu(m_fptr, size+1)));
	return dynamic_cast<ImageHdu&>(*m_hdus[size].get());
}

template<typename ...Ts>
BintableHdu& MefFile::init_bintable_ext(std::string name, const ColumnInfo<Ts>&... header) {
	Cfitsio::Hdu::create_bintable_extension(m_fptr, name, header...);
	const auto size = m_hdus.size();
	m_hdus.push_back(std::unique_ptr<RecordHdu>(new BintableHdu(m_fptr, size+1)));
	return dynamic_cast<BintableHdu&>(*m_hdus[size].get());
}

template<typename ...Ts>
BintableHdu& MefFile::assign_bintable_ext(std::string name, const Column<Ts>&... data) {
	Cfitsio::Hdu::create_bintable_extension(m_fptr, name, data...);
	const auto size = m_hdus.size();
	m_hdus.push_back(std::unique_ptr<RecordHdu>(new BintableHdu(m_fptr, size+1)));
	return dynamic_cast<BintableHdu&>(*m_hdus[size].get());
}


}
}

#endif
