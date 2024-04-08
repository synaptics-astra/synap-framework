#include "synap/zip_tool.hpp"

#include "synap/file_utils.hpp"
#include "synap/logging.hpp"

#include "miniz.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace synaptics {
namespace synap {

struct ZipTool::Private
{
    mz_zip_archive _zip{};
    vector<mz_zip_archive_file_stat> _archives{};

    bool init()
    {
        auto num_file = mz_zip_reader_get_num_files(&_zip);
        if (!num_file) {
            LOGE << "No files in archive"<< num_file;
            return false;
        }

        for (auto i = 0; i != num_file; ++i) {
            mz_zip_archive_file_stat stat;
            /* Returns detailed information about an archive file entry. */
            if (!mz_zip_reader_file_stat(&_zip, i, &stat)) {
                LOGE << "failed to retrieve stat for index " << i;
                return false;
            }
            _archives.emplace_back(stat);
        }
        return true;
    }
};

ZipTool::ZipTool(): d(new Private)
{
}

ZipTool::~ZipTool()
{
    close();
}

bool ZipTool::close()
{
    bool result = mz_zip_reader_end(&d->_zip);
    d.reset(new Private);
    return result;
}

bool ZipTool::open(string zipFileName)
{
    if (d->_zip.m_zip_type != MZ_ZIP_TYPE_INVALID) {
        LOGE << "ZipTool is already open, please close first" << endl;
        return false;
    }
    if (!mz_zip_reader_init_file(&d->_zip, zipFileName.c_str(), 0)) {
        LOGE << "Failed to read ZIP file " << zipFileName;
        return false;
    }
    return d->init();
}

bool ZipTool::open(const void *zipData, size_t zipDataSize)
{
    d->_zip = mz_zip_archive{};
    if (!mz_zip_reader_init_mem(&d->_zip, zipData, zipDataSize, 0)) {
        LOGE << "Failed to read ZIP data";
        return false;
    }
    return d->init();
}

bool ZipTool::extract_all(const std::string & out_directory) {
    bool success = true;
    create_directory(out_directory);
    for (const auto & arch : d->_archives) {
        const string out_path = out_directory + "/" + arch.m_filename;
        if (arch.m_is_directory) {
            create_directory(out_path);
            continue;
        }
        if (!mz_zip_reader_extract_to_file(&d->_zip, arch.m_file_index, out_path.c_str(), 0)) {
            LOGE << "Cannot extract " << out_path;
            success = false;
        }
    }
    return success;
}

std::vector<ZipTool::Archive> ZipTool::get_archive_list() const
{
    std::vector<Archive> result;
    for (const auto & arch : d->_archives)  {
        if (!arch.m_is_directory)
            result.push_back({arch.m_file_index, arch.m_filename,
                              static_cast<size_t>(arch.m_uncomp_size)});
    }
    return result;
}


vector<uint8_t> ZipTool::extract_archive(const std::string & name)
{
    const mz_zip_archive_file_stat * found{};
    for (const auto & arch : d->_archives) {
        if (name == arch.m_filename) {
            found = &arch;
            break;
        }
    }
    if (!found) {
        LOGE << "Not found in archive: " << name;
        return vector<uint8_t>{};
    }
    vector<uint8_t> extracted(found->m_uncomp_size);
    const bool success = mz_zip_reader_extract_to_mem_no_alloc(
        &d->_zip, found->m_file_index, &extracted[0], found->m_uncomp_size, 0, 0, 0);
    if (!success) {
        LOGE << "Failed to extract archive " << found->m_file_index << ": " << name;
        return vector<uint8_t>{};
    }
    return extracted;
}

bool ZipTool::extract_archive(uint32_t index, uint8_t * archiveData)
{
    const mz_zip_archive_file_stat * found{};
    for (const auto & arch : d->_archives) {
        if (index == arch.m_file_index) {
            found = &arch;
            break;
        }
    }
    if (!found) {
        LOGE << "Index " << index << " not found in archive";
        return false;
    }
    const bool success = mz_zip_reader_extract_to_mem_no_alloc(
        &d->_zip, found->m_file_index, archiveData, found->m_uncomp_size, 0, 0, 0);
    if (!success) {
        LOGE << "Failed to extract archive " << found->m_file_index << ": " << found->m_filename;
        return false;
    }
    return true;
}


}  // namespace synap
}  // namespace synaptics
