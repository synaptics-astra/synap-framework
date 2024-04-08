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

#include "synap/detector.hpp"
#include "synap/classifier.hpp"
#include "synap/logging.hpp"
#include "json.hpp"


using namespace std;

namespace synaptics {
namespace synap {

static constexpr int json_dump_indent = 2;

using json = nlohmann::json;

void to_json(json& j, const Dim2d& p);
void to_json(json& j, const Landmark& p);
void to_json(json& j, const Rect& p);
void to_json(json& j, const Detector::Result::Item& p);
void to_json(json& j, const Detector::Result& p);
void to_json(json& j, const Classifier::Result& p);


void to_json(json& j, const Dim2d& p)
{
    j = json{{"x", p.x}, {"y", p.y}};
}


void to_json(json& j, const Landmark& p)
{
    j = json{{"x", p.x}, {"y", p.y}, {"z", p.z}, {"visibility", p.visibility}};
}


void to_json(json& j, const Rect& p)
{
    j = json{
        {"origin", p.origin},
        {"size", p.size}
    };
}


void to_json(json& j, const Detector::Result::Item& p)
{
    json lms = json{ { "points", p.landmarks } };
    j = json{
        {"class_index", p.class_index},
        {"confidence", p.confidence},
        {"bounding_box", p.bounding_box},
        {"landmarks", lms }
    };
}

void to_json(json& j, const Detector::Result& p)
{
    j = json{
        {"success", p.success},
        {"items", p.items}
    };
}


void to_json(json& j, const Classifier::Result::Item& p)
{
    j = json{
        {"class_index", p.class_index},
        {"confidence", p.confidence}
    };
}


void to_json(json& j, const Classifier::Result& p)
{
    j = json{
        {"success", p.success},
        {"items", p.items}
    };
}


std::string to_json_str(const Detector::Result& p) {
    json j;
    to_json(j, p);
    return j.dump(json_dump_indent);
}


std::string to_json_str(const Classifier::Result& p)
{
    json j;
    to_json(j, p);
    return j.dump(json_dump_indent);
}


}  // namespace synap
}  // namespace synaptics
