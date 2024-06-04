#include "synap/arg_parser.hpp"
#include "synap/file_utils.hpp"
#include "synap/zip_tool.hpp"
#if SYNAP_EBG_ENABLE
#include "synap/ebg_utils.h"
#endif

#ifdef SYNAP_PYBIND_ENABLE
#include <pybind11/embed.h>
namespace py = pybind11;
using namespace py::literals;
#endif

#include "json.hpp"
#include <iostream>
#include <thread>

using namespace std;
using namespace synaptics::synap;

using json = nlohmann::ordered_json;
static constexpr int json_dump_indent = 2;

static bool onnx_info(const uint8_t * data, size_t size)
{
    #ifdef SYNAP_PYBIND_ENABLE

    string onnx_model_path = "/tmp/test-onnx.onnx";
    binary_file_write(onnx_model_path, data, size);
    cout << "dump to " << onnx_model_path << endl;

    py::scoped_interpreter guard{};

    py::module_ onnx = py::module_::import("onnx");
    py::object model = onnx.attr("load")(onnx_model_path);

    cout << "model input:" << endl;

    py::object input = model.attr("graph").attr("input");
    for (auto i : input) {
        py::print(i.attr("name"), i.attr("type").attr("tensor_type"));
    }

    cout << "model output:" << endl;

    py::object output = model.attr("graph").attr("output");
    for (auto o : output) {
        py::print(o.attr("name"), o.attr("type").attr("tensor_type"));
    }

    cout << "per-layer nodes:" << endl;

    py::object nodes = model.attr("graph").attr("node");
    for (auto n : nodes) {
        py::print(n.attr("name"), n.attr("op_type"), n.attr("input"), n.attr("output"));
    }

    string cmd = "rm -rf " + onnx_model_path;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        cerr << "cannot remove model" << onnx_model_path << endl;
    }

    #endif

    return true;
}


int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "Synap Model Info", "[options]");
    string model_file = args.get("-m", "<file> synap model", "model.synap");
    int archive_id = stoi(args.get("--id", "<n> archive id", "-1"));
    args.check_help("--help", "Show help");

    const vector<uint8_t> zipData = binary_file_read(model_file);
    ZipTool ztool;
    if (!ztool.open(zipData.data(), zipData.size())) {
        cerr << "Failed to open zip archive" << endl;
        return -1;
    }

    auto archives = ztool.get_archive_list();
    if (archives.empty()) {
        cerr << "it is valid synap model" << endl;
        return -1;
    }

    cout << "synap model :" << endl;
    for (const auto & arch: archives) {
        cout << "id " << arch.index << " " << arch.name << " size " << arch.size << "\t " << endl;
    }

    if (archive_id < 0) {
        return 0;
    }

    cout << "inspect archive id " << archive_id << "..." << endl;
    if (archive_id >= archives.size()) {
        cerr << "Invalid archive id" << endl;
        return -1;
    }

    const auto & arch = archives[archive_id];
    cout << "id " << arch.index << " " << arch.name << " size " << arch.size << "\t " << endl;

    vector<uint8_t> uncompressed(arch.size);
    if (!ztool.extract_archive(arch.index, uncompressed.data())) {
        cerr << "Failed to extract archive" << endl;
        return -1;
    }

    if (uncompressed.data()[0] == 'E' && uncompressed.data()[1] == 'B' &&
        uncompressed.data()[2] == 'G' && uncompressed.data()[3] == 'X') {
        cout << "npu compiled model" << endl;
#if SYNAP_EBG_ENABLE
        ebg_info(uncompressed.data(), arch.size, true);
#endif
        return 0;
    }

    if (uncompressed.data()[4] == 'o' && uncompressed.data()[5] == 'n' &&
        uncompressed.data()[6] == 'n' && uncompressed.data()[7] == 'x') {
        cout << "onnx compiled model" << endl;
        onnx_info(uncompressed.data(), arch.size);
        return 0;
    }

    json j;
    try {
        j = json::parse(uncompressed.data());
    } catch (json::parse_error& e) {
        cerr << "id " << archive_id << " is not a json file" << endl;
        // TODO: maybe tf parser
        return -1;
    }

    cout << setw(json_dump_indent) << j << endl;

    return 0;
}
