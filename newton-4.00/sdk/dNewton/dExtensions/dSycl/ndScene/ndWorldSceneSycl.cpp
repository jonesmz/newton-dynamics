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

#include "ndCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndWorld.h"
#include "ndModel.h"
#include "ndWorldScene.h"
#include "ndSyclContext.h"
#include "ndBodyDynamic.h"
#include "ndSkeletonList.h"
#include "ndWorldSceneSycl.h"
#include "ndDynamicsUpdate.h"
#include "ndBodyParticleSet.h"
#include "ndJointBilateralConstraint.h"

ndWorldSceneSycl::ndWorldSceneSycl(const ndWorldScene& src, bool selectCpu)
	:ndWorldScene(src)
{
	m_context = new ndSyclContext(selectCpu);
}

ndWorldSceneSycl::~ndWorldSceneSycl()
{
	if (m_context)
	{
		delete m_context;
	}
}

bool ndWorldSceneSycl::IsValid() const
{
	return m_context->IsValid();
}

#if 0
//ndCudaContext* ndWorldSceneSycl::GetContext()
//{
//	return this;
//}


bool ndWorldSceneSycl::IsGPU() const
{
	ndAssert(0);
	return false;
	//return ndCudaContext::IsValid();
}

double ndWorldSceneSycl::GetGPUTime() const
{
	ndAssert(0);
	return 0;
	//return ndCudaContext::GetGPUTime();
}

void ndWorldSceneSycl::Begin()
{
	ndAssert(0);
	//ndWorldScene::Begin();
	//ndCudaContext::Begin();
}

void ndWorldSceneSycl::End()
{
	ndAssert(0);
	//ndCudaContext::End();
	//ndWorldScene::End();
}

void ndWorldSceneSycl::LoadBodyData()
{
	auto CopyBodies = ndMakeObject::ndFunction([this](ndInt32 threadIndex, ndInt32 threadCount)
	{
		ndAssert(0);
		//D_TRACKTIME();
		//const ndCudaVector minBox(ndFloat32(1.0e15f));
		//const ndCudaVector maxBox(ndFloat32(-1.0e15f));
		//
		//ndArray<ndCudaBodyProxy>& data = m_bodyBufferCpu;
		//ndCudaSpatialVector* const transformBufferCpu = GetTransformBuffer();
		//
		//ndArray<ndBodyKinematic*>& bodyArray = GetActiveBodyArray();
		//const ndStartEnd startEnd(bodyArray.GetCount(), threadIndex, threadCount);
		//for (ndInt32 i = startEnd.m_start; i < startEnd.m_end; ++i)
		//{
		//	ndCudaSpatialVector transform;
		//	ndBodyKinematic* const body = bodyArray[i];
		//	ndCudaBodyProxy& proxi = data[i];
		//
		//	// Get thansform and velocity
		//	const ndVector massMatrix(body->GetMassMatrix());
		//	const ndQuaternion rotation(body->GetRotation());
		//	const ndVector invInertia(body->GetInvInertia());
		//	const ndVector dampCoef(body->GetCachedDamping());
		//	const ndVector position(body->GetGlobalGetCentreOfMass());
		//	const ndVector veloc(body->GetVelocity());
		//	const ndVector omega(body->GetOmega());
		//
		//	proxi.m_mass = ndCudaVector(massMatrix.m_x, massMatrix.m_y, massMatrix.m_z, massMatrix.m_w);
		//	proxi.m_rotation = ndCudaQuat(rotation.m_x, rotation.m_y, rotation.m_z, rotation.m_w);
		//	proxi.m_posit = ndCudaVector(position.m_x, position.m_y, position.m_z, position.m_w);
		//	proxi.m_invIntertia = ndCudaVector(invInertia.m_x, invInertia.m_y, invInertia.m_z, invInertia.m_w);
		//	proxi.m_dampCoef = ndCudaVector(dampCoef.m_x, dampCoef.m_y, dampCoef.m_z, dampCoef.m_w);
		//	proxi.m_veloc = ndCudaVector(veloc.m_x, veloc.m_y, veloc.m_z, veloc.m_w);
		//	proxi.m_omega = ndCudaVector(omega.m_x, omega.m_y, omega.m_z, omega.m_w);
		//
		//	// Get scene manager data
		//	const ndShapeInstance& collision = body->GetCollisionShape();
		//	const ndShape* const shape = collision.GetShape();
		//
		//	const ndVector scale(collision.GetScale());
		//	const ndVector obbSize(shape->GetObbSize());
		//	const ndVector obbOrigin(shape->GetObbOrigin());
		//	const ndVector localPosition (collision.GetLocalMatrix().m_posit);
		//	const ndQuaternion localRotation(collision.GetLocalMatrix());
		//	const ndQuaternion alignRotation(collision.GetAlignmentMatrix());
		//
		//	proxi.m_minAabb = minBox;
		//	proxi.m_maxAabb = maxBox;
		//	proxi.m_obbSize = ndCudaVector(obbSize.m_x, obbSize.m_y, obbSize.m_z, obbSize.m_w);
		//	proxi.m_obbOrigin = ndCudaVector(obbOrigin.m_x, obbOrigin.m_y, obbOrigin.m_z, obbOrigin.m_w);
		//	proxi.m_scale = ndCudaVector(scale.m_x, scale.m_y, scale.m_z, scale.m_w);
		//	proxi.m_localPosition = ndCudaVector(localPosition.m_x, localPosition.m_y, localPosition.m_z, localPosition.m_w);
		//	proxi.m_localRotation = ndCudaQuat(localRotation.m_x, localRotation.m_y, localRotation.m_z, localRotation.m_w);
		//	proxi.m_alignRotation = ndCudaQuat(alignRotation.m_x, alignRotation.m_y, alignRotation.m_z, alignRotation.m_w);
		//
		//	transform.m_linear = proxi.m_posit;
		//	transform.m_angular = proxi.m_rotation;
		//	transformBufferCpu[i] = transform;
		//}
	});

	ndAssert(0);


	//const ndArray<ndBodyKinematic*>& bodyArray = GetActiveBodyArray();
	//const ndInt32 cpuBodyCount = bodyArray.GetCount();
	//ndArray<ndCudaBodyProxy>& bodyBufferCpu = m_bodyBufferCpu;
	//bodyBufferCpu.SetCount(cpuBodyCount);
	//ndCudaContext::ResizeBuffers(cpuBodyCount);
	//
	//ParallelExecute(CopyBodies);
	//
	//ndCudaContext::LoadBodyData(&bodyBufferCpu[0], cpuBodyCount);
}

void ndWorldSceneSycl::GetBodyTransforms()
{
	D_TRACKTIME();
	ndAssert(0);
	//auto GetTransform = [] __device__(const cuSceneInfo& info, int frameCount)
	//{
	//	int index = threadIdx.x + blockDim.x * blockIdx.x;
	//	if (index < (info.m_bodyArray.m_size - 1))
	//	{
	//		cuBodyProxy* src = info.m_bodyArray.m_array;
	//		cuSpatialVector* dst = (frameCount & 1) ? info.m_transformBuffer0.m_array : info.m_transformBuffer1.m_array;
	//
	//		dst[index].m_linear = src[index].m_posit;
	//		dst[index].m_angular = src[index].m_rotation;
	//	}
	//};
	//
	//cudaStream_t stream = m_context->m_solverComputeStream;
	//cuSceneInfo* const infoGpu = m_context->m_sceneInfoGpu;
	//
	//ndInt32 threads = m_context->m_bodyBufferGpu.GetCount() - 1;
	//ndInt32 blocks = (threads + D_THREADS_PER_BLOCK - 1) / D_THREADS_PER_BLOCK;
	//CudaGetBodyTransforms << <blocks, D_THREADS_PER_BLOCK, 0, stream >> > (GetTransform, *infoGpu, m_context->m_frameCounter);
	//
	////cuHostBuffer<cuSpatialVector>& cpuBuffer = m_context->m_transformBufferCpu0;
	////cuDeviceBuffer<cuSpatialVector>& gpuBuffer = m_context->m_transformBufferGpu0;
	////gpuBuffer.WriteData(&cpuBuffer[0], cpuBuffer.GetCount() - 1, stream);
}

void ndWorldSceneSycl::UpdateTransform()
{
	D_TRACKTIME();
	ndAssert(0);


	//ndCudaContext::UpdateTransform();
	//auto SetTransform = ndMakeObject::ndFunction([this](ndInt32 threadIndex, ndInt32 threadCount)
	//
	//D_TRACKTIME();
	//const ndArray<ndBodyKinematic*>& bodyArray = GetActiveBodyArray();
	//const ndCudaSpatialVector* const data = GetTransformBuffer();
	//const ndStartEnd startEnd(bodyArray.GetCount() - 1, threadIndex, threadCount);
	//for (ndInt32 i = startEnd.m_start; i < startEnd.m_end; ++i)
	//{
	//	ndBodyKinematic* const body = bodyArray[i];
	//	const ndCudaSpatialVector& transform = data[i];
	//	const ndVector position(transform.m_linear.x, transform.m_linear.y, transform.m_linear.z, ndFloat32(1.0f));
	//	const ndQuaternion rotation(ndVector(transform.m_angular.x, transform.m_angular.y, transform.m_angular.z, transform.m_angular.w));
	//	body->SetMatrixAndCentreOfMass(rotation, position);
	//	
	//	body->m_transformIsDirty = true;
	//	UpdateTransformNotify(threadIndex, body);
	//}
	//);
	//arallelExecute(SetTransform);
}

void ndWorldSceneSycl::BalanceScene()
{
	ndWorldScene::BalanceScene();
}

void ndWorldSceneSycl::CalculateContacts()
{
	//ndWorldScene::CalculateContacts();
}

void ndWorldSceneSycl::UpdateBodyList()
{
	bool bodyListChanged = m_bodyList.IsListDirty();

	ndWorldScene::UpdateBodyList();
	if (bodyListChanged)
	{
		LoadBodyData();
	}
	
	ndAssert(0);
	//ValidateContextBuffers();
}

void ndWorldSceneSycl::FindCollidingPairs()
{
	ndWorldScene::FindCollidingPairs();
}

void ndWorldSceneSycl::ApplyExtForce()
{
	ndWorldScene::ApplyExtForce();
}

void ndWorldSceneSycl::InitBodyArray()
{
	ndAssert(0);
	//ndWorldScene::InitBodyArray();
	//ndCudaContext::InitBodyArray();
}
#endif