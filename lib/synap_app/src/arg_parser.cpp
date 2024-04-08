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

#include "synap/arg_parser.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <unistd.h>
#include "synap/file_utils.hpp"

using namespace std;

namespace synaptics {
namespace synap {


ArgParser::ArgParser(int& argc, char** argv, string title, string summary)
  : _title(title), _summary(summary)
{
    _program_name = argv[0];
    for (int i = 1; i < argc; ++i)
        _tokens.push_back(string(argv[i]));
    _token_used.resize(_tokens.size());
}


bool ArgParser::has(string option, string help)
{
    add_opt(option, help, "");
    auto it = find(_tokens.begin(), _tokens.end(), option);
    if (it == _tokens.end())
        return false;
    update_options_usage(it);
    return true;
}


string ArgParser::get(string option, string help, string default_val)
{
    add_opt(option, help, default_val);
    auto it = find(_tokens.begin(), _tokens.end(), option);
    if (it != _tokens.end()) {
        update_options_usage(it);
        if (++it != _tokens.end()) {
            update_options_usage(it);
            return *it;
        }
    }
    return default_val;
}


const string& ArgParser::get()
{
    static const string end_of_params;
    if (_next_unnamed_ix < _tokens.size())
        return _tokens[_next_unnamed_ix++];
    return end_of_params;
}


void ArgParser::add_opt(const string& opt, const string& help, const string& default_value)
{
    for (const auto& o : _opts) {
        if (o.name == opt) {
            return;
        }
    }
    _opts.emplace_back(std::move(opt), std::move(help), std::move(default_value));
}


void ArgParser::update_options_usage(const vector<string>::iterator& it)
{
    int index = distance(_tokens.begin(), it);
    _next_unnamed_ix = max(index + 1, _next_unnamed_ix);
    _token_used[index] = true;
}

string ArgParser::help()
{
    stringstream help_string;
    if (!_title.empty()) {
        help_string << _title << endl;
    }
    help_string << "usage: " << _program_name << " " << _summary << endl;
    size_t max_len = 0;
    for (const auto& opt : _opts) {
        max_len = max(max_len, opt.name.size() + opt.help_pre.size());
    }
    for (const auto& opt : _opts) {
        help_string << "    " << setw(max_len + 2) << left << opt.name + opt.help_pre << opt.help;
        if (!opt.default_value.empty()) {
            help_string << " [" << opt.default_value << "] ";
        }
        help_string << endl;
    }
    return help_string.str();
}


string ArgParser::illegal_options()
{
    string illegal_options;

    // Advance until the end of options if not yet there (ignore the remaining args)
    while (_next_unnamed_ix < _tokens.size() && _tokens[_next_unnamed_ix][0] == '-') {
        ++_next_unnamed_ix;
    }

    // Collect all unused options
    for (int i = 0; i < _next_unnamed_ix; i++) {
        if (!_token_used[i]) {
            illegal_options += _tokens[i] + " ";
        }
    }

    return illegal_options;
}

void ArgParser::check_help(string option, string help_string, string help_notes)
{
    if (has(std::move(option), std::move(help_string))) {
        cerr << help() << help_notes << endl;
        exit(0);
    }
    else if (!illegal_options().empty()) {
        cerr << "Illegal option(s): " << illegal_options() << endl << help() << help_notes << endl;
        exit(1);
    }
}


ArgParser::Opt::Opt(string opt_name, string opt_help, string opt_default)
  : name{opt_name}, help{opt_help}, default_value{opt_default}
{
    if (!help.empty() && help[0] == '<') {
        size_t split = help.find(">");
        if (split != string::npos) {
            help_pre = " " + help.substr(0, split + 1);
            help = help.substr(help.find_first_not_of(" ", split + 1));
        }
    }
}


void validate_model_arg(std::string& model, std::string& nb, std::string& meta, const std::string& default_dir)
{
    // Pick model name from -m or legacy --nb command line option
    if (!model.empty() && !nb.empty()) {
        cerr << "Error: -m and --nb options cannot be specified together" << endl;
        exit(1);
    }
    if (model.empty()) {
        model = nb;
    }
    nb.clear();
    
    // If no model specified use .synap or .nb from current directory first, then default directory
    if (model.empty()) {
        vector<string> dirs = { "" };
        if (!default_dir.empty()) {
            dirs.emplace_back(default_dir);
        }
        for (const string& dir: dirs) {
            model = dir + "model.synap";
            if (file_exists(model)) {
                break;
            }
#ifdef SYNAP_FILE_BASED_BUNDLE
            if (file_exists(dir + "bundle.json")) {
                model = dir.empty()? "." : dir;
                break;
            }
#endif
            if (file_exists(dir + "model.nb")) {
                model = dir + "model.nb";
                if (meta.empty()) {
                    meta = dir + "model.json";
                }
                break;
            }
        }
    }

    bool have_metafile = !meta.empty();
    if (directory_exists(model)) {
#ifdef SYNAP_FILE_BASED_BUNDLE
        // Directory containing an unzipped bundle model
        if (have_metafile) {
            cerr << "Error: metafile cannot be specified when model is a directory" << endl;
            exit(1);
        }
        return;

#else
        cerr << "Error: model '" << model << "' is a directory" << endl;
        exit(1);
#endif
    }
    if (!file_exists(model)) {
        cerr << "Error: can't find model file: " << model << endl;
        exit(1);
    }
    bool is_synap_model = filename_extension(model) == "synap";
    if (is_synap_model && have_metafile) {
        cerr << "Error: SyNAP models don't require a metafile" << endl;
        exit(1);
    }
    else if (!is_synap_model && !have_metafile) {
        cerr << "Error: metafile required for non-SyNAP models" << endl;
        exit(1);
    }
    if (have_metafile && !file_exists(meta)) {
        cerr << "Error: can't find meta file: " << meta << endl;
        exit(1);
    }
}


bool output_redirected() {
  return !isatty(fileno(stdout));
}


string read_input() {
  if (!isatty(fileno(stdin))) {
      return string(istreambuf_iterator<char>{cin}, {});
  }
  return "";
}


}  // namespace synap
}  // namespace synaptics
