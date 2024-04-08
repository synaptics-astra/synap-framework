/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace synaptics {
namespace synap {

/// @return filename extension
std::string filename_extension(const std::string& file_name);

/// @return filename without extension and path
std::string filename_without_extension(const std::string& file_name);

/// @return filename path
std::string filename_path(const std::string& file_name);

/// @return true if file exists
bool file_exists(const std::string& file_name);

/// Search if a file with a given name is present in the specifed path
/// or in any parent directory up to the specified number of levels
/// @return filename path if found, else empty string
std::string file_find_up(const std::string& file_name, const std::string& path, int levels = 2);

/// @return true if directory exists
bool directory_exists(const std::string& directory_name);

/// Read content of a file
std::string file_read(const std::string& file_name);

/// Read content of a binary file
std::vector<uint8_t> binary_file_read(const std::string& file_name);

/// Write data to a binary file
/// @return true if success
bool binary_file_write(const std::string& file_name, const void* data, size_t size);

/// wrapper for non-C++-17 systems std::filesystem::create_directory()
bool create_directory(const std::string& out_dir);

/// wrapper for non-C++-17 systems std::filesystem::create_directories()
bool create_directories(const std::string& out_dir);

}  // namespace synap
}  // namespace synaptics
