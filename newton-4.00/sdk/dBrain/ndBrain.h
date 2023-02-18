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

#ifndef _ND_BRAIN_H__
#define _ND_BRAIN_H__

#include "ndBrainStdafx.h"
#include "ndBrainLayer.h"
#include "ndBrainTrainerBase.h"

class ndBrain: public ndArray<ndBrainLayer*>
{
	public: 
	ndBrain();
	ndBrain(const ndBrain& src);
	~ndBrain();

	ndInt32 GetInputSize() const;
	ndInt32 GetOutputSize() const;

	bool Load(const char* const pathName);
	void Save(const char* const pathName) const;
	void CopyFrom(const ndBrain& src);

	void BeginAddLayer();
	void EndAddLayer();
	bool Compare(const ndBrain& src) const;

	ndBrainLayer* AddLayer(ndBrainLayer* const layer);
	void InitGaussianWeights(ndReal mean, ndReal variance);

	void* m_memory;
	ndInt32 m_memorySize;
	bool m_isReady;
};

#endif 
