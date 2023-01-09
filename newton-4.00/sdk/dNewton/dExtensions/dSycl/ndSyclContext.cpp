/* Copyright (c) <2003-2021> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include <ndSyclStdafx.h>
#include "ndSyclUtils.h"
#include "ndSyclContext.h"
#include "ndSyclContextImpl.h"

using namespace sycl;

ndSyclContext::ndSyclContext(bool selectCpu)
	:m_impl(nullptr)
{
	EnumDevices(selectCpu);
}

ndSyclContext::~ndSyclContext()
{
	if (m_impl)
	{
		delete m_impl;
	}
}

void* ndSyclContext::operator new (size_t size)
{
	return ndSyclMalloc(size);
}

void ndSyclContext::operator delete (void* ptr)
{
	ndSyclFree(ptr);
}

void ndSyclContext::SetMemoryAllocators(ndMemAllocCallback alloc, ndMemFreeCallback free)
{
	ndSyclSetMemoryAllocators(alloc, free);
}

bool ndSyclContext::IsValid() const 
{
	return m_impl ? true : false;
}

const char* ndSyclContext::GetStringId() const
{
	ndAssert(m_impl);
	return m_impl->GetStringId();
}

void ndSyclContext::EnumDevices(bool selectCpu)
{
	std::vector<device> devices;
	std::vector<platform> platforms(platform::get_platforms());
	for (int i = 0; i < platforms.size(); ++i)
	{
		platform& plat = platforms[i];
		std::vector<device> devList(plat.get_devices());
		for (int j = 0; j < devList.size(); ++j)
		{
			device& dev = devList[j];
			if (selectCpu)
			{
				if (dev.is_cpu())
				{
					devices.push_back(dev);
				}
			}
			else
			{
				if (dev.is_gpu())
				{
					devices.push_back(dev);
				}
			}
		}
	}

	if (devices.size())
	{
		device bestDevice = devices[0];
		for (int i = 0; i < devices.size(); ++i)
		{
			std::string paltfromeName(devices[i].get_platform().get_info<info::platform::name>());
		}

		m_impl = new ndSyclContextImpl(bestDevice);
	}
}


#if 0
double ndCudaContext::GetGPUTime() const
{
	return IsValid() ? m_implement->GetTimeInSeconds() : 0.0;
}

ndCudaSpatialVector* ndCudaContext::GetTransformBuffer()
{
	return m_implement->GetTransformBuffer();
}

void ndCudaContext::Begin()
{
	m_implement->Begin();
}

void ndCudaContext::End()
{
	m_implement->End();
}

void ndCudaContext::ResizeBuffers(int size)
{
	m_implement->ResizeBuffers(size);
}

void ndCudaContext::LoadBodyData(const ndCudaBodyProxy* const src, int size)
{
	m_implement->LoadBodyData(src, size);
}

void ndCudaContext::ValidateContextBuffers()
{
	m_implement->ValidateContextBuffers();
}

void ndCudaContext::InitBodyArray()
{
	m_implement->InitBodyArray();
}

void ndCudaContext::IntegrateBodies(float timestep)
{
	m_implement->IntegrateBodies(timestep);
}

void ndCudaContext::IntegrateUnconstrainedBodies(float timestep)
{
	m_implement->IntegrateUnconstrainedBodies(timestep);
}

void ndCudaContext::UpdateTransform()
{
	m_implement->UpdateTransform();
}
#endif