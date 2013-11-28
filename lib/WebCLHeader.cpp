/*
** Copyright (c) 2013 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include "WebCLConfiguration.hpp"
#include "WebCLHeader.hpp"
#include "WebCLTypes.hpp"

static const char *image2d = "image2d_t";

WebCLHeader::WebCLHeader(WebCLConfiguration &cfg)
    : cfg_(cfg)
    , indentation_("    ")
    , level_(0)
{
    // nothing
}

WebCLHeader::~WebCLHeader()
{
}

void WebCLHeader::emitHeader(std::ostream &out, const WebCLAnalyser::KernelList &kernels)
{
    out << "\n";
    emitIndentation(out);
    out << "/* WebCL Validator JSON header\n";
    emitIndentation(out);
    out << "{\n";

    ++level_;
    emitVersion(out);
    out << ",\n";
    emitKernels(out, kernels);
    out << "\n";

    --level_;
    emitIndentation(out);
    out << "}\n";
    emitIndentation(out);
    out << "*/\n\n";
}

void WebCLHeader::emitNumberEntry(
    std::ostream &out,
    const std::string &key, int value)
{
    emitIndentation(out);
    out << "\"" << key << "\" : " << value;
}

void WebCLHeader::emitStringEntry(
    std::ostream &out,
    const std::string &key, const std::string &value)
{
    emitIndentation(out);
    out << "\"" << key << "\" : \"" << value << "\"";
}

void WebCLHeader::emitVersion(std::ostream &out)
{
    emitStringEntry(out, "version", "1.0");
}

void WebCLHeader::emitHostType(
    std::ostream &out,
    const std::string &key, const std::string &type)
{
    using namespace WebCLTypes;
    
    HostTypes::const_iterator i = hostTypes().find(type);
    if (i == hostTypes().end()) {
        assert(false && "WebCLRestrictor should have prevented using unsupported kernel parameter types");
        return;
    }

    const std::string &hostType = i->second;
    emitStringEntry(out, key, hostType);
}

void WebCLHeader::emitParameter(
    std::ostream &out,
    const std::string &parameter, int index, const std::string &type,
    const Fields &fields)
{
    emitIndentation(out);
    out << "\"" << parameter << "\"" << " :\n";
    ++level_;
    emitIndentation(out);
    out << "{\n";
    ++level_;

    emitNumberEntry(out, "index", index);
    out << ",\n";
    emitHostType(out, "host-type", type);

    for (Fields::const_iterator i = fields.begin(); i != fields.end(); ++i) {
        out << ",\n";
        emitStringEntry(out, i->first, i->second);
    }
    out << "\n";

    --level_;
    emitIndentation(out);
    out << "}";
    --level_;
}

void WebCLHeader::emitBuiltinParameter(
    std::ostream &out,
    const WebCLAnalyser::KernelArgInfo &parameter, int index, const std::string &type)
{
    Fields fields;

    // Add "access" : "qualifier" field for images.
    if (!type.compare(image2d)) {
        std::string access = "read_only";

        switch (parameter.imageKind) {
        case WebCLAnalyser::READABLE_IMAGE:
            access = "read_only";
            break;
        case WebCLAnalyser::WRITABLE_IMAGE:
            access = "write_only";
            break;
        default:
            // ImageSafetyHandler errors on this for all image parameters which are passed on to image access functions;
            // others can be considered read_only with 0 reads as it is not possible to access images directly
            access = "read_only";
            break;
        }

        fields["access"] = access;
    }

    emitParameter(out, parameter.name, index, type, fields);
}

void WebCLHeader::emitSizeParameter(
    std::ostream &out,
    const WebCLAnalyser::KernelArgInfo &parameter, int index)
{
    const std::string parameterName = cfg_.getNameOfSizeParameter(parameter.name);
    const std::string typeName = cfg_.sizeParameterType_;
    emitParameter(out, parameterName, index, typeName);
}

void WebCLHeader::emitArrayParameter(
    std::ostream &out,
    const WebCLAnalyser::KernelArgInfo &parameter, int index)
{
    emitIndentation(out);
    out << "\"" << parameter.name << "\"" << " :\n";
    ++level_;
    emitIndentation(out);
    out << "{\n";
    ++level_;

    emitNumberEntry(out, "index", index);
    out << ",\n";

    emitStringEntry(out, "host-type", "cl_mem");
    out << ",\n";
    emitHostType(out, "host-element-type", parameter.pointeeTypeName);
    out << ",\n";

    switch (parameter.pointerKind) {
    case WebCLAnalyser::GLOBAL_POINTER:
        emitStringEntry(out, "address-space", "global");
        break;
    case WebCLAnalyser::CONSTANT_POINTER:
        emitStringEntry(out, "address-space", "constant");
        break;
    case WebCLAnalyser::LOCAL_POINTER:
        emitStringEntry(out, "address-space", "local");
        break;
    default:
        assert(false && "WebCLKernelHandler::run() should have rejected array of private memory kernel parameter");
        break;
    }
    out << ",\n";

    emitStringEntry(out, "size-parameter", cfg_.getNameOfSizeParameter(parameter.name));
    out << "\n";

    --level_;
    emitIndentation(out);
    out << "}";
    --level_;
}

void WebCLHeader::emitKernel(std::ostream &out, const WebCLAnalyser::KernelInfo &kernel)
{
    emitIndentation(out);
    out << "\"" << kernel.name << "\"" << " :\n";
    ++level_;
    emitIndentation(out);
    out << "{\n";
    ++level_;

    unsigned index = 0;
    for (std::vector<WebCLAnalyser::KernelArgInfo>::const_iterator i = kernel.args.begin();
        i != kernel.args.end(); ++i) {
        const WebCLAnalyser::KernelArgInfo &parameter = *i;

        if (i != kernel.args.begin())
            out << ",\n";

        if (WebCLTypes::supportedBuiltinTypes().count(parameter.reducedTypeName)) {
            // images and samplers
            emitBuiltinParameter(out, parameter, index, parameter.reducedTypeName);
        } else if (parameter.pointerKind != WebCLAnalyser::NOT_POINTER) {
            // memory objects
            emitArrayParameter(out, parameter, index);
            ++index;
            out << ",\n";
            emitSizeParameter(out, parameter, index);
        } else {
            // primitives
            emitParameter(out, parameter.name, index, parameter.reducedTypeName);
        }
        ++index;
    }
    out << "\n";

    --level_;
    emitIndentation(out);
    out << "}";
    --level_;
}

void WebCLHeader::emitKernels(std::ostream &out, const WebCLAnalyser::KernelList &kernels)
{
    emitIndentation(out);
    out << "\"kernels\" :\n";
    ++level_;
    emitIndentation(out);
    out << "{\n";
    ++level_;

    for (WebCLAnalyser::KernelList::const_iterator i = kernels.begin(); i != kernels.end(); ++i) {
        const WebCLAnalyser::KernelInfo &kernel = *i;

        if (i != kernels.begin())
            out << ",\n";
        emitKernel(out, kernel);
    }
    out << "\n";

    --level_;
    emitIndentation(out);
    out <<"}";
    --level_;
}

void WebCLHeader::emitIndentation(std::ostream &out) const
{
    for (unsigned int i = 0; i < level_; ++i)
        out << indentation_;
}
