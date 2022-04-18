
// Get SDK from https ://github.com/KhronosGroup/OpenCL-SDK/releases
#include <CL/cl.hpp>
#include <iostream>

#define GLSL(input) #input
static const char source[] = GLSL(
kernel void WriteValue(int offset, global int* output)
{
	size_t global_id = get_global_id(0);
	output[global_id] = offset + global_id;
});

int main()
{
	// Get list of OpenCL platforms.
	std::vector<cl::Platform> platform;
	cl::Platform::get(&platform);

	cl::Context cl_context;
	cl::Device	cl_device;

	bool found = false;
	for (auto p = platform.begin(); !found && p != platform.end(); p++)
	{
		std::string platname = p->getInfo<CL_PLATFORM_NAME>();
		std::cout << "Platform Name: " << platname << "\n";

		std::vector<cl::Device> deviceList;
		p->getDevices(CL_DEVICE_TYPE_GPU, &deviceList);
		for (auto d = deviceList.begin(); !found && d != deviceList.end(); d++)
		{
			if (!d->getInfo<CL_DEVICE_AVAILABLE>()) continue;

			std::string name = d->getInfo<CL_DEVICE_NAME>();
			std::cout << "Found: " << name << "\n";

			found		= true;
			cl_device	= *d;
			cl_context	= cl::Context(cl_device);
		}
	}

	std::cout << "CL Device and Context initialised \n";
	cl::Program program(cl_context, cl::Program::Sources( 1, std::make_pair(source, strlen(source))));

	std::vector<cl::Device> device_vector = { cl_device };
	cl_int programBuildRes = program.build(device_vector);
	if (programBuildRes != CL_SUCCESS)
	{
		std::cout	<< "OpenCL GLSL compilation error: \n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl_device)	<< "\n";
		return 1;
	}

	cl::CommandQueue queue(cl_context, cl_device);
	cl::Kernel		 writeValueKernel(program, "WriteValue");

	const int			kernelValueOffset	= 10;
	const size_t		N					= 1024;
	std::vector<cl_int> kernelOutputBuffer_staging(N);
	cl::Buffer			kernelOutputBuffer(cl_context, CL_MEM_READ_WRITE, N * sizeof(cl_int));

	writeValueKernel.setArg(0, static_cast<cl_int>(kernelValueOffset));
	writeValueKernel.setArg(1, kernelOutputBuffer);
	
	cl_int kernelRunRes = queue.enqueueNDRangeKernel(writeValueKernel, cl::NullRange, N, cl::NullRange);
	if (kernelRunRes != CL_SUCCESS)
		std::cout << "Kernel Failed to run.\n";
	
	// Get result back to host.
	cl_int kernelBufferReadRes = queue.enqueueReadBuffer(kernelOutputBuffer, CL_TRUE, 0, N * sizeof(cl_int), kernelOutputBuffer_staging.data());
	if (kernelBufferReadRes != CL_SUCCESS)
		std::cout << "Buffer Failed to read.\n";

	if (kernelOutputBuffer_staging[1] == kernelValueOffset + 1)
		std::cout << "This program ran successfully.\n";
	else
		std::cout << "This program failed.\n";

	return 0;
}