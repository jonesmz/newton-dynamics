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

#ifndef _ND_DEEP_BRAIN_TRAINING_OPERATOR_H__
#define _ND_DEEP_BRAIN_TRAINING_OPERATOR_H__

#include "ndDeepBrainStdafx.h"
#include "ndDeepBrainMatrix.h"
#include "ndDeepBrainInstance.h"

class ndDeepBrainTrainingOperator: public ndClassAlloc
{
	public: 
	ndDeepBrainTrainingOperator(ndDeepBrain* const brain);
	ndDeepBrainTrainingOperator(const ndDeepBrainTrainingOperator& src);
	virtual ~ndDeepBrainTrainingOperator();

	ndDeepBrain* GetBrain() const;
	void SetMiniBatchSize(ndInt32 m_miniBatchSize);
	ndFloat32 CalculateMeanSquareError(const ndDeepBrainVector& groundTruth) const;
	virtual void Optimize(const ndDeepBrainMatrix& inputBatch, const ndDeepBrainMatrix& groundTruth, ndReal learnRate, ndInt32 steps) = 0;

	protected:
	ndDeepBrainInstance m_instance;
	ndInt32 m_miniBatchSize;

	friend class ndDeepBrainInstance;
};

inline ndDeepBrain* ndDeepBrainTrainingOperator::GetBrain() const
{
	return m_instance.GetBrain();
}
#endif 

