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
///
/// Synap command line application for NB files
///


#include "synap/arg_parser.hpp"
#include "synap/file_utils.hpp"
#include "synap/label_info.hpp"
// For json parsing and validation purpose
#include "synap/metadata.hpp"

#include "synap/ebg_utils.h"

#include <cstring>

#include <iostream>
#include <iomanip>
#include <string>

using namespace std;
using namespace synaptics::synap;

int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "Synap nb file utility program", "[options]");
    const string nb_filename = args.get("--nb", "<file> Network binary model file");
    const string meta_filename = args.get("--meta", "<file> Network json meta file");
    const string labels_filename = args.get("--labels", "<file> json labels file");
    const string ebg_filename = args.get("--to-ebg", "<file> Executable binary graph model file");
    bool profile = args.has("--profile", "generate EBG for profiling");
    bool print_info = args.has("--info", "Show EBG information");
    bool extended_info = args.has("--ext-info", "Show extended EBG information");
    print_info |= extended_info;

    args.check_help("--help", "Show help", "");

    if (!meta_filename.empty()) {
        string ebg_meta = file_read(meta_filename);
        cout << "parsing metafile: " << meta_filename << endl;
        NetworkMetadata meta = load_metadata(ebg_meta.c_str());
        if (!meta.valid) {
            cerr << "Failed to load metadata" << endl;
        }
    }

    if (!labels_filename.empty()) {
        cout << "parsing labels: " << labels_filename << endl;
        LabelInfo info(labels_filename);
    }

    if (nb_filename.empty()) {
        cerr << args.help();
        return 1;
    }

    // load the model
    vector<uint8_t> nb = binary_file_read(nb_filename);
    if (nb.empty()) {
        cerr << "Failed to load nbg file: " << nb_filename << endl;
        return 2;
    }

    if (!ebg_filename.empty()) {
        if (profile)
            cout << "Generating EBG for layer by layer profiling" << endl;
        // Generate the EBG from NBG
        uint8_t * ebg_buffer{};
        size_t ebg_size = nbg_to_ebg(nb.data(), nb.size(), &ebg_buffer, profile);
        if (ebg_size == 0 || ebg_buffer == nullptr) {
            cerr << "NBG to EBG conversion failed" << endl;
            return 3;
        }
        if (!binary_file_write(ebg_filename, ebg_buffer, ebg_size)) {
            cerr << "Cannot write EBG file to " << ebg_filename << endl;
            return 4;
        }
        cout << "\nCreated EBG model: " << ebg_filename << endl;
        if (print_info)
            ebg_info(ebg_buffer, ebg_size, extended_info);
        free(ebg_buffer);
        return 0;
    }

    if (print_info)
        ebg_info(nb.data(), nb.size(), extended_info);
    return 0;
}
