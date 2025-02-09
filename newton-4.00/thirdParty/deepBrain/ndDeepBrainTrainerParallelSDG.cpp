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

#include "ndDeepBrainStdafx.h"
#include "ndDeepBrain.h"
#include "ndDeepBrainLayer.h"
#include "ndDeepBrainTrainerParallelSDG.h"

ndDeepBrainTrainerParallelSDG::LocalData::LocalData(const ndDeepBrainTrainerSDG& src)
	:ndDeepBrain(*src.GetBrain())
	,ndDeepBrainTrainerSDG((ndDeepBrain*)this, src.GetRegularizer())
	,m_averageError(0.0f)
{
}

void ndDeepBrainTrainerParallelSDG::LocalData::CopyTranspose(const ndArray<ndDeepBrainMatrix*>& src)
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	for (ndInt32 i = layers.GetCount() - 1; i >= 1; --i)
	{
		ndDeepBrainMatrix& transposeMatrix = *m_weightsLayersTranspose[i];
		transposeMatrix.Set(*src[i]);
	}
}

ndDeepBrainTrainerParallelSDG::ndDeepBrainTrainerParallelSDG(ndDeepBrain* const brain, ndReal regularizer, ndInt32 threads)
	:ndDeepBrainTrainerSDG(brain, regularizer)
	,ndThreadPool("neuralNet")
	,m_inputBatch(nullptr)
	,m_groundTruth(nullptr)
	,m_learnRate(0.0f)
	,m_steps(0)
{
	threads = ndMin(threads, D_MAX_THREADS_COUNT);
	SetThreadCount(threads);

	for (ndInt32 i = 0; i < threads; i++)
	{
		m_threadData.PushBack(new LocalData(*this));
	}
}

ndDeepBrainTrainerParallelSDG::~ndDeepBrainTrainerParallelSDG()
{
	Finish();
	for (ndInt32 i = 0; i < GetThreadCount(); i++)
	{
		delete m_threadData[i];
	}
}

void ndDeepBrainTrainerParallelSDG::ThreadFunction()
{
	Begin();
	Optimize();
	End();
}

void ndDeepBrainTrainerParallelSDG::Optimize(const ndDeepBrainMatrix& inputBatch, const ndDeepBrainMatrix& groundTruth, ndReal learnRate, ndInt32 steps)
{
	m_steps = steps;
	m_learnRate = learnRate;
	m_inputBatch = &inputBatch;
	m_groundTruth = &groundTruth;
	TickOne();
	Sync();
}

void ndDeepBrainTrainerParallelSDG::AverageWeights()
{
	const ndInt32 threads = m_threadData.GetCount();
	const ndReal weightFactor = 1.0f / threads;
	
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	for (ndInt32 j = layers.GetCount() - 1; j >= 0; --j)
	{
		ndDeepBrainLayer& layer = *layers[j];
		ndDeepBrainVector& bias = layer.GetBias();
	
		layer.Set(0);
		bias.Set(0.0f);
		for (ndInt32 i = 0; i < threads; ++i)
		{
			const ndDeepBrainLayer& srcLayer = *(*m_threadData[i]->GetBrain())[j];
			bias.Add(bias, srcLayer.GetBias());
			for (ndInt32 k = 0; k < layer.GetOuputSize(); k++)
			{
				layer[k].Add(layer[k], srcLayer[k]);
			}
		}
		
		layer.GetBias().ScaleSet(layer.GetBias(), weightFactor);
		for (ndInt32 k = 0; k < layer.GetOuputSize(); k++)
		{
			layer[k].ScaleSet(layer[k], weightFactor);
		}
	}
}

void ndDeepBrainTrainerParallelSDG::Optimize()
{
	ndAssert(m_inputBatch->GetCount() == m_groundTruth->GetCount());
	ndAssert(m_output.GetCount() == (*m_groundTruth)[0].GetCount());
	
	ndDeepBrain bestNetwork(*m_instance.GetBrain());
	ndReal bestCost = 1.0e10f;
	
	ndInt32 index = 0;
	ndInt32 batchCount = (m_inputBatch->GetCount() + m_miniBatchSize - 1) / m_miniBatchSize;
	ndArray<ndInt32> randomizeVector;
	randomizeVector.SetCount(m_inputBatch->GetCount());
	for (ndInt32 i = 0; i < m_inputBatch->GetCount(); ++i)
	{
		randomizeVector[i] = i;
	}
	
	ndInt32 movingAverageIndex = 0;
	ndFloat32 movingAverageError = 0.0f;
	for (ndInt32 i = 0; i < m_steps; ++i)
	{
		const ndInt32 batchStart = index * m_miniBatchSize;
		const ndInt32 batchSize = index != (batchCount - 1) ? m_miniBatchSize : m_inputBatch->GetCount() - batchStart;
	
		auto CalculateGradients = ndMakeObject::ndFunction([this, batchStart, batchSize, &randomizeVector](ndInt32 threadIndex, ndInt32 threadCount)
		{
			LocalData& optimizer = *m_threadData[threadIndex];
			optimizer.m_averageError = 0.0f;
	
			const ndStartEnd startEnd(batchSize, threadIndex, threadCount);
			for (ndInt32 i = startEnd.m_start; i < startEnd.m_end; ++i)
			{
				ndInt32 k = randomizeVector[batchStart + i];
				const ndDeepBrainVector& input = (*m_inputBatch)[k];
				const ndDeepBrainVector& truth = (*m_groundTruth)[k];
				optimizer.m_averageError += optimizer.TrainingStep(m_learnRate, input, truth);
			}
		});
		ParallelExecute(CalculateGradients);
	
		AverageWeights();
		ApplyWeightTranspose();
	
		ndReal averageError = 0.0f;
		for(ndInt32 j = GetThreadCount() - 1; j >= 0; --j)
		{
			averageError += m_threadData[j]->m_averageError;
			m_threadData[j]->GetBrain()->CopyFrom(*GetBrain());
			m_threadData[j]->CopyTranspose(m_weightsLayersTranspose);
		}
	
		movingAverageIndex += batchSize;
		movingAverageError += averageError;
	
		averageError = ndSqrt(averageError / batchSize);
		ndExpandTraceMessage("%f %d\n", averageError, i);
	
		index = (index + 1) % batchCount;
		if (index == 0)
		{
			randomizeVector.RandomShuffle(randomizeVector.GetCount());
			movingAverageError = ndSqrt(movingAverageError / movingAverageIndex);
			if (movingAverageError < bestCost)
			{
				bestCost = movingAverageError;
				bestNetwork.CopyFrom(*m_instance.GetBrain());
			}
			movingAverageIndex = 0;
			movingAverageError = 0.0f;
		}
	}
	
	m_instance.GetBrain()->CopyFrom(bestNetwork);
}