/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2022 Synaptics Incorporated. All rights reserved.
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

#include "synap/arg_parser.hpp"

#include "synap/file_utils.hpp"
#include "synap/timer.hpp"
#include "synap/zip_tool.hpp"

#include <iostream>
#include <iomanip>
#include <string>

using namespace std;
using namespace synaptics::synap;

int extract_all_from_file_to_file(const string & zipfname, const string & outputfolder)
{
    ZipTool ztool;
    if (!ztool.open(zipfname))
        return 3;
    if (!ztool.extract_all(outputfolder))
        return 4;
    return 0;
}

int extract_all_from_mem_to_file(const string & zipfname, const string & outputfolder)
{
    const vector<uint8_t> zipData = binary_file_read(zipfname);
    ZipTool ztool;
    if (!ztool.open(zipData.data(), zipData.size()))
        return 3;
    if (!ztool.extract_all(outputfolder))
        return 4;
    return 0;
}

int extract_all_from_mem_to_mem(const string & zipfname, const string & outputfolder)
{
    Timer t;
    const vector<uint8_t> zipData = binary_file_read(zipfname);
    cout << "binary_file_read " << t.get() << endl;
    ZipTool ztool;
    t.start();
    if (!ztool.open(zipData.data(), zipData.size()))
        return 3;
    cout << "ztool.open " << t.get() << endl;

    auto archives = ztool.get_archive_list();
    for (const auto & arch: archives) {
        const string outpath = "./" + outputfolder + "/" + arch.name;
        cout << "id " << arch.index << " size " << arch.size << "\t " << arch.name << endl;

        vector<uint8_t> uncompressed(arch.size);
        if (!ztool.extract_archive(arch.index, uncompressed.data()))
            return 4;
        std::size_t found = outpath.find_last_of("/\\");
        if (create_directories(outpath.substr(0, found))) {
            cout << "directories created: " << outpath.substr(0, found);
        }
        if (!binary_file_write(outpath, uncompressed.data(), uncompressed.size())) {
            return 6;
        }
    }
    return 0;
}


int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "ZipTool testing program", "[options]");
    const int test_id = std::stoi( args.get("-m", "<int> test ID"));
    const string zipfname = args.get("-f", "<file> zip file");
    const string outputfolder = args.get("-o", "output folder for test artefacts");

    args.check_help("--help", "Show help", "");

    if (zipfname.empty()) {
        cerr << "Needs an input filename" << endl;
        return 1;
    }
    if (outputfolder.empty()) {
        cerr << "Needs an output folder" << endl;
        return 2;
    }

    switch (test_id) {
    case 0:
        return extract_all_from_file_to_file(zipfname, outputfolder);
    case 1:
        return extract_all_from_mem_to_file(zipfname, outputfolder);
    case 2:
        return extract_all_from_mem_to_mem(zipfname, outputfolder);
    default:
        cerr << "unknown test ID" << endl;
        return 10;
    }

    return 0;
}
