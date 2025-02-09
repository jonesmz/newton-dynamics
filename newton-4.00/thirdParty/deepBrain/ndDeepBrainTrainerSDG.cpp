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
#include "ndDeepBrainTrainerSDG.h"

ndDeepBrainTrainerSDG::ndDeepBrainTrainerSDG(ndDeepBrain* const brain, ndReal regularizer)
	:ndDeepBrainTrainingOperator(brain)
	,m_output()
	,m_zDerivative()
	,m_biasGradients()
	,m_weightGradients()
	,m_weightGradientsPrefixScan()
	,m_weightsLayersTranspose()
	,m_regularizer(regularizer)
{
	ndAssert(regularizer >= 0.0f);
	PrefixScan();

	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	for (ndInt32 i = 0; i < layers.GetCount(); ++i)
	{
		const ndDeepBrainLayer& layer = *layers[i];
		m_weightsLayersTranspose.PushBack(new ndDeepBrainMatrix(layer.GetInputSize(), layer.GetOuputSize()));
		m_weightsLayersTranspose[i]->SetTranspose(layer);
	}
}

ndDeepBrainTrainerSDG::ndDeepBrainTrainerSDG(const ndDeepBrainTrainerSDG& src)
	:ndDeepBrainTrainingOperator(src)
	,m_output(src.m_output)
	,m_zDerivative(src.m_zDerivative)
	,m_biasGradients(src.m_biasGradients)
	,m_weightGradients(src.m_weightGradients)
	,m_weightGradientsPrefixScan(src.m_weightGradientsPrefixScan)
	,m_weightsLayersTranspose()
	,m_regularizer(src.m_regularizer)
{
	for (ndInt32 i = 0; i < src.m_weightsLayersTranspose.GetCount(); i++)
	{
		m_weightsLayersTranspose.PushBack(new ndDeepBrainMatrix(*src.m_weightsLayersTranspose[i]));
	}
}

ndDeepBrainTrainerSDG::~ndDeepBrainTrainerSDG()
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	for (ndInt32 i = 0; i < layers.GetCount(); i++)
	{
		delete (m_weightsLayersTranspose[i]);
	}
}

void ndDeepBrainTrainerSDG::PrefixScan()
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	m_instance.CalculatePrefixScan();

	ndDeepBrainVector& instance_z = m_instance.GetOutPut();
	m_zDerivative.SetCount(instance_z.GetCount());
	m_biasGradients.SetCount(instance_z.GetCount());

	m_zDerivative.Set(0.0f);
	m_biasGradients.Set(0.0f);
	m_output.SetCount(layers[layers.GetCount() - 1]->GetOuputSize());

	m_weightGradientsPrefixScan.SetCount(layers.GetCount());
	for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
	{
		ndDeepBrainLayer* const layer = layers[i];
		ndInt32 stride = (layer->GetInputSize() + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
		m_weightGradientsPrefixScan[i] = stride * layer->GetOuputSize();
	}

	ndInt32 sum = 0;
	for (ndInt32 i = 0; i < m_weightGradientsPrefixScan.GetCount(); ++i)
	{
		ndInt32 count = m_weightGradientsPrefixScan[i];
		m_weightGradientsPrefixScan[i] = sum;
		sum += count;
	}
	m_weightGradients.SetCount(sum);
	m_weightGradients.Set(0.0f);
}

void ndDeepBrainTrainerSDG::MakePrediction(const ndDeepBrainVector& input)
{
	m_instance.MakePrediction(input, m_output);
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());

	ndDeepBrainVector& instance_z = m_instance.GetOutPut();
	const ndDeepBrainPrefixScan& preFixScan = m_instance.GetPrefixScan();
	for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
	{
		ndDeepBrainLayer* const layer = layers[i];
		const ndDeepBrainMemVector z(&instance_z[preFixScan[i + 1]], layer->GetOuputSize());
		ndDeepBrainMemVector zDerivative(&m_zDerivative[preFixScan[i + 1]], layer->GetOuputSize());
		layer->ActivationDerivative(z, zDerivative);
	}
}

void ndDeepBrainTrainerSDG::BackPropagateOutputLayer(const ndDeepBrainVector& groundTruth)
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	const ndInt32 layerIndex = layers.GetCount() - 1;

	ndDeepBrainLayer* const ouputLayer = layers[layerIndex];
	const ndInt32 inputCount = ouputLayer->GetInputSize();
	const ndInt32 outputCount = ouputLayer->GetOuputSize();
	const ndDeepBrainVector& instance_z = m_instance.GetOutPut();
	const ndDeepBrainPrefixScan& preFixScan = m_instance.GetPrefixScan();
	ndDeepBrainMemVector biasGradients(&m_biasGradients[preFixScan[layerIndex + 1]], outputCount);
	const ndDeepBrainMemVector z(&instance_z[preFixScan[layerIndex + 1]], outputCount);
	const ndDeepBrainMemVector zDerivative(&m_zDerivative[preFixScan[layerIndex + 1]], outputCount);

	biasGradients.Sub(z, groundTruth);
	biasGradients.Mul(biasGradients, zDerivative);

	const ndInt32 stride = (inputCount + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
	ndReal* weightGradientPtr = &m_weightGradients[m_weightGradientsPrefixScan[layerIndex]];
	const ndDeepBrainMemVector z0(&instance_z[preFixScan[layerIndex]], inputCount);
	for (ndInt32 i = 0; i < outputCount; ++i)
	{
		ndDeepBrainMemVector weightGradient(weightGradientPtr, inputCount);
		ndFloat32 gValue = biasGradients[i];
		weightGradient.ScaleSet(z0, gValue);
		weightGradientPtr += stride;
	}
}

void ndDeepBrainTrainerSDG::BackPropagateCalculateBiasGradient(ndInt32 layerIndex)
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	ndDeepBrainLayer* const layer = layers[layerIndex + 1];
	const ndDeepBrainPrefixScan& preFixScan = m_instance.GetPrefixScan();

	const ndDeepBrainMemVector biasGradients1(&m_biasGradients[preFixScan[layerIndex + 2]], layer->GetOuputSize());
	ndDeepBrainMemVector biasGradients(&m_biasGradients[preFixScan[layerIndex + 1]], layer->GetInputSize());
	const ndDeepBrainMatrix& matrix = *m_weightsLayersTranspose[layerIndex + 1];
	const ndDeepBrainMemVector zDerivative(&m_zDerivative[preFixScan[layerIndex + 1]], layer->GetInputSize());

	matrix.Mul(biasGradients1, biasGradients);
	biasGradients.Mul(biasGradients, zDerivative);
}

void ndDeepBrainTrainerSDG::BackPropagateHiddenLayer(ndInt32 layerIndex)
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	BackPropagateCalculateBiasGradient(layerIndex);

	ndDeepBrainLayer* const layer = layers[layerIndex];
	const ndDeepBrainVector& instance_z = m_instance.GetOutPut();
	const ndDeepBrainPrefixScan& preFixScan = m_instance.GetPrefixScan();

	const ndDeepBrainMemVector biasGradients(&m_biasGradients[preFixScan[layerIndex + 1]], layer->GetOuputSize());

	const ndInt32 inputCount = layer->GetInputSize();
	const ndInt32 stride = (inputCount + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
	ndReal* weightGradientPtr = &m_weightGradients[m_weightGradientsPrefixScan[layerIndex]];

	const ndDeepBrainMemVector z0(&instance_z[preFixScan[layerIndex]], inputCount);
	for (ndInt32 i = 0; i < layer->GetOuputSize(); ++i)
	{
		ndDeepBrainMemVector weightGradient(weightGradientPtr, inputCount);
		ndFloat32 gValue = biasGradients[i];
		weightGradient.ScaleSet(z0, gValue);
		weightGradientPtr += stride;
	}
}

void ndDeepBrainTrainerSDG::UpdateWeights(ndReal learnRate)
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
	{
		ndDeepBrainLayer* const layer = layers[i];
		const ndInt32 inputSize = layer->GetInputSize();
		const ndInt32 outputSize = layer->GetOuputSize();
		const ndDeepBrainPrefixScan& preFixScan = m_instance.GetPrefixScan();
	
		const ndDeepBrainMemVector biasGradients(&m_biasGradients[preFixScan[i + 1]], outputSize);
		layer->GetBias().ScaleAdd(biasGradients, -learnRate);
	
		const ndInt32 weightGradientStride = (inputSize + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
		ndReal* weightGradientPtr = &m_weightGradients[m_weightGradientsPrefixScan[i]];
	
		ndDeepBrainMatrix& weightMatrix = *layer;
		ndReal regularizer = GetRegularizer();
		for (ndInt32 j = 0; j < outputSize; ++j)
		{
			ndDeepBrainVector& weightVector = weightMatrix[j];
			const ndDeepBrainMemVector weightGradients(weightGradientPtr, inputSize);
			weightVector.ScaleAdd(weightVector, -regularizer);
			weightVector.ScaleAdd(weightGradients, -learnRate);
			weightGradientPtr += weightGradientStride;
		}
	}
}

void ndDeepBrainTrainerSDG::ApplyWeightTranspose()
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());
	for (ndInt32 i = layers.GetCount() - 1; i >= 1; --i)
	{
		ndDeepBrainLayer* const layer = layers[i];
		ndDeepBrainMatrix& weightMatrix = *layer;
		m_weightsLayersTranspose[i]->SetTranspose(weightMatrix);
	}
}

void ndDeepBrainTrainerSDG::BackPropagate(const ndDeepBrainVector& groundTruth)
{
	const ndArray<ndDeepBrainLayer*>& layers = (*m_instance.GetBrain());

	BackPropagateOutputLayer(groundTruth);
	for (ndInt32 i = layers.GetCount() - 2; i >= 0; --i)
	{
		BackPropagateHiddenLayer(i);
	}
}

ndReal ndDeepBrainTrainerSDG::TrainingStep(ndReal learnRate, const ndDeepBrainVector& input, const ndDeepBrainVector& groundTruth)
{
	MakePrediction(input);
	BackPropagate(groundTruth);
	UpdateWeights(learnRate);
	ndFloat32 leastSquareError = CalculateMeanSquareError(groundTruth);
	return leastSquareError;
}

void ndDeepBrainTrainerSDG::Optimize(const ndDeepBrainMatrix& inputBatch, const ndDeepBrainMatrix& groundTruth, ndReal learnRate, ndInt32 steps)
{
	ndFloatExceptions exception;
	ndAssert(inputBatch.GetCount() == groundTruth.GetCount());
	ndAssert(m_output.GetCount() == groundTruth[0].GetCount());
	
	ndDeepBrain bestNetwork(*m_instance.GetBrain());
	ndReal bestCost = 1.0e10f;
	
	ndInt32 index = 0;
	ndInt32 batchCount = (inputBatch.GetCount() + m_miniBatchSize - 1) / m_miniBatchSize;
	ndArray<ndInt32> randomizeVector;
	randomizeVector.SetCount(inputBatch.GetCount());
	for (ndInt32 i = 0; i < inputBatch.GetCount(); ++i)
	{
		randomizeVector[i] = i;
	}
	
	ndInt32 movingAverageIndex = 0;
	ndFloat32 movingAverageError = 0.0f;
	for (ndInt32 i = 0; i < steps; ++i)
	{
		const ndInt32 batchStart = index * m_miniBatchSize;
		const ndInt32 batchSize = index != (batchCount - 1) ? m_miniBatchSize : inputBatch.GetCount() - batchStart;
	
		ndReal averageError = 0.0f;
		for (ndInt32 j = 0; j < batchSize; ++j)
		{
			ndInt32 k = randomizeVector[batchStart + j];
			const ndDeepBrainVector& input = inputBatch[k];
			const ndDeepBrainVector& truth = groundTruth[k];
			averageError += TrainingStep(learnRate, input, truth);
		}
		ApplyWeightTranspose();
	
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