#ifndef WEBCLVALIDATOR_OPENCLVALIDATOR
#define WEBCLVALIDATOR_OPENCLVALIDATOR

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>
#include <iterator>

class OpenCLValidator
{
public:

    OpenCLValidator()
        : numPlatforms_(0), platform_(0), numDevices_(0), device_(0)
        , context_(0), queue_(0), program_(0) {
    }

    virtual ~OpenCLValidator() {
        // Intel's implementation requires cleanup if clBuildProgram
        // fails. Otherwise we get these errors:
        // - pure virtual method called
        // - terminate called without an active exception
        // - Segmentation fault (core dumped)
        if (program_)
            clReleaseProgram(program_);
        if (queue_)
            clReleaseCommandQueue(queue_);
        if (context_)
            clReleaseContext(context_);
    }

    virtual unsigned int getNumPlatforms() {
        if (clGetPlatformIDs(0, NULL, &numPlatforms_) != CL_SUCCESS)
            return 0;
        return numPlatforms_;
    }

    virtual bool createPlatform(unsigned int platform) {
        if (platform >= numPlatforms_)
            return false;

        const unsigned int maxPlatforms = 10;
        if (platform >= maxPlatforms)
            return false;

        cl_platform_id platforms[maxPlatforms];
        if (clGetPlatformIDs(numPlatforms_, platforms, NULL) != CL_SUCCESS)
            return false;
        platform_ = platforms[platform];
        return true;
    }

    virtual unsigned int getNumDevices() {
        if (clGetDeviceIDs(platform_, CL_DEVICE_TYPE_ALL,
                           0, NULL, &numDevices_) != CL_SUCCESS) {
            return 0;
        }
        return numDevices_;
    }

    virtual bool createDevice(unsigned int device) {
        if (device >= numDevices_)
            return false;

        const unsigned int maxDevices = 10;
        if (device >= maxDevices)
            return false;

        cl_device_id devices[maxDevices];
        if (clGetDeviceIDs(platform_, CL_DEVICE_TYPE_ALL,
                           numDevices_, devices, NULL) != CL_SUCCESS) {
            return false;
        }
        device_ = devices[device];
        return true;
    }

    virtual bool createContext() {
        if (context_) {
            clReleaseContext(context_);
            context_ = 0;
        }

        cl_context_properties properties[3] = {
            CL_CONTEXT_PLATFORM,
            (cl_context_properties)platform_,
            0
        };
        context_ = clCreateContext(properties, 1, &device_, NULL, NULL, NULL);
        return context_ != 0;
    }

    virtual bool createQueue() {
        if (queue_) {
            clReleaseCommandQueue(queue_);
            queue_ = 0;
        }
        queue_ =  clCreateCommandQueue(context_, device_, 0, NULL);
        return queue_ != 0;
    }

    virtual bool createProgram() {
        if (program_) {
            clReleaseProgram(program_);
            program_ = 0;
        }

        std::cin >> std::noskipws;
        std::string code((std::istream_iterator<char>(std::cin)), std::istream_iterator<char>());
        const char *source = code.c_str();
        program_ = clCreateProgramWithSource(context_, 1, &source, NULL, NULL);
        return program_ != 0;
    }

    virtual bool buildProgram(std::string options) {
        options.append("-Werror");
        return clBuildProgram(program_, 1, &device_, options.c_str(), NULL, NULL) == CL_SUCCESS;
    }

    std::string getPlatformName() {
        char name[100];
        if (clGetPlatformInfo(platform_, CL_PLATFORM_NAME, sizeof(name), &name, NULL) != CL_SUCCESS)
            return "?";
        return name;
    }

    std::string getDeviceName() {
        char name[100];
        if (clGetDeviceInfo(device_, CL_DEVICE_NAME, sizeof(name), &name, NULL) != CL_SUCCESS)
            return "?";
        return name;
    }

    virtual void printProgramLog() {
        char log[10 * 1024];
        if (clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL) == CL_SUCCESS) {
            std::cerr << log;
        }
    }

protected:

    cl_uint numPlatforms_;
    cl_platform_id platform_;
    cl_uint numDevices_;
    cl_device_id device_;

    cl_context context_;
    cl_command_queue queue_;
    cl_program program_;
};

#endif // WEBCLVALIDATOR_OPENCLVALIDATOR
