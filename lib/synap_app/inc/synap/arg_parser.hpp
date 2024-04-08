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
#include <algorithm>
#include <memory>
#include <string>
#include <vector>


namespace synaptics {
namespace synap {

/// Very simple command option parser
class ArgParser {
public:
    ArgParser(int& argc, char** argv, std::string title = "", std::string summary = "");

    /// @return true if the specified option is present in the command line
    bool has(std::string option, std::string help = "");

    /// @return the next unnamed command line parameter if present, else ""
    /// Should be called after all named option have been read.
    const std::string& get();

    /// @return the value associated to the specified option if present
    std::string get(std::string option, std::string help = "", std::string default_val = "");

    /// @return help string
    std::string help();

    /// @return illegal (unparsed) options, or empty string if all right
    std::string illegal_options();

    /// Handle help and check for invalid options.
    /// If help was requested or invalid options specified will exit and not return.
    void check_help(std::string option, std::string help, std::string help_notes = "");

private:
    struct Opt {
        Opt(std::string opt_name, std::string opt_help, std::string default_value);
        std::string name;
        std::string help_pre;
        std::string help;
        std::string default_value;
    };
    void update_options_usage(const std::vector<std::string>::iterator& it);
    void add_opt(const std::string& opt, const std::string& help, const std::string& default_value);

    std::string _program_name;
    std::string _title;
    std::string _summary;
    std::vector<std::string> _tokens;
    std::vector<bool> _token_used;
    std::vector<Opt> _opts;
    int _next_unnamed_ix{0};
};


/// Helper function to validate model name from command-line args and transition fom SyNAP 2 to 3
/// If an empty model name is specified, it will be replaced with default model name.
/// If invalid model name arguments specified will exit and not return.
/// @param model: in: model parameter, out: model file name
/// @param nb: in: legacy nb parameter, out: empty
/// @param meta: legacy meta parameter, out: meta file name if any
/// @param default_dir: default directory
void validate_model_arg(std::string& model, std::string& nb, std::string& meta, const std::string& default_dir = "");


/// Read input parameters from standard input if redirected
std::string read_input();


/// Check if stdout redirected
bool output_redirected();


}  // namespace synap
}  // namespace synaptics
