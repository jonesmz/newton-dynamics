/* Copyright (c) <2003-2022> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _ND_DEEP_BRAIN_INSTANCE_H__
#define _ND_DEEP_BRAIN_INSTANCE_H__

#include "ndDeepBrainStdafx.h"
#include "ndDeepBrainVector.h"

class ndDeepBrain;
class ndDeepBrainLayer;
class ndDeepBrainTrainingOperator;

class ndDeepBrainInstance: public ndClassAlloc
{
	public: 
	ndDeepBrainInstance(ndDeepBrain* const brain);
	ndDeepBrainInstance(const ndDeepBrainInstance& src);
	~ndDeepBrainInstance();

	void CalculatePrefixScan();
	ndDeepBrain* GetBrain() const;
	ndDeepBrainVector& GetOutPut();
	const ndDeepBrainVector& GetOutPut() const;
	const ndDeepBrainPrefixScan& GetPrefixScan() const;

	void MakePrediction(const ndDeepBrainVector& input, ndDeepBrainVector& output);
	void MakePrediction(ndThreadPool& threadPool, const ndDeepBrainVector& input, ndDeepBrainVector& output);

	protected:
	ndDeepBrainVector m_z;
	ndDeepBrainPrefixScan m_zPrefixScan;
	ndDeepBrain* m_brain;

	//friend class ndDeepBrainTrainingOperator;
	//friend class ndDeepBrainGradientDescendTrainingOperator;
	//friend class ndDeepBrainParallelGradientDescendTrainingOperatorOld;
};

inline ndDeepBrain* ndDeepBrainInstance::GetBrain() const
{
	return m_brain;
}

inline ndDeepBrainVector& ndDeepBrainInstance::GetOutPut()
{
	return m_z;
}

inline const ndDeepBrainVector& ndDeepBrainInstance::GetOutPut() const
{
	return m_z;
}

inline const ndDeepBrainPrefixScan& ndDeepBrainInstance::GetPrefixScan() const
{
	return m_zPrefixScan;
}

#endif 

