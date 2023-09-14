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

#include "ndBrainStdafx.h"
#include "ndBrain.h"
#include "ndBrainLoss.h"
#include "ndBrainLayer.h"
#include "ndBrainVector.h"
#include "ndBrainMatrix.h"
#include "ndBrainTrainer.h"

class ndBrainTrainer::ndLayersVariables : public ndClassAlloc
{
	public:
	ndLayersVariables(ndBrainLayer* const layer)
		:ndClassAlloc()
		//,m_z()
		,m_layer(layer)
	{
		//m_z.SetCount(m_layer->GetOutputSize());
	}

	void ClearGradAcc()
	{
		ndTrace(("this is wrong)"));
		//m_layer->ClearGradAcc(m_gradBiasAcc, m_gradWeightAcc);
	}

	//ndBrainVector m_z;
	//ndBrainVector m_gradBiasAcc;
	//ndBrainMatrix m_gradWeightAcc;
	ndBrainVector m_gradBias;
	ndBrainMatrix m_gradWeight;
	ndBrainLayer* m_layer;
};


ndBrainTrainer::ndBrainTrainer(ndBrain* const brain)
	:ndClassAlloc()
	,m_layers()
	,m_z()
	,m_zDerivative()
	,m_biasGradients()
	,m_weightGradients()
	,m_biasGradientsAcc()
	,m_biasGradient_u()
	,m_biasGradient_v()
	,m_weightGradient_u()
	,m_weightGradient_v()
	,m_brain(brain)
	,m_beta(0.999f)
	,m_alpha(0.9f)
	,m_epsilon(1.0e-8f)
	,m_betaAcc(m_beta)
	,m_alphaAcc(m_alpha)
	,m_weighDecayRegularizer(ndReal (1.0e-5f))
	,m_model(m_adam)
{
	//PrefixScan();
	for (ndInt32 i = 0; i < m_brain->GetCount(); ++i)
	{
		m_layers.PushBack(new ndLayersVariables((*m_brain)[i]));
	}
}

ndBrainTrainer::ndBrainTrainer(const ndBrainTrainer& src)
	:ndClassAlloc()
	,m_z(src.m_z)
	,m_zDerivative(src.m_zDerivative)
	,m_biasGradients(src.m_biasGradients)
	,m_weightGradients(src.m_weightGradients)
	,m_biasGradientsAcc(src.m_biasGradientsAcc)
	,m_biasGradient_u(src.m_biasGradient_u)
	,m_biasGradient_v(src.m_biasGradient_v)
	,m_weightGradient_u(src.m_weightGradient_u)
	,m_weightGradient_v(src.m_weightGradient_v)
	,m_brain(src.m_brain)
	,m_beta(src.m_beta)
	,m_alpha(src.m_alpha)
	,m_epsilon(src.m_epsilon)
	,m_betaAcc(src.m_betaAcc)
	,m_alphaAcc(src.m_alphaAcc)
	,m_weighDecayRegularizer(src.m_weighDecayRegularizer)
	,m_model(src.m_model)
{
	ndAssert(0);
	for (ndInt32 i = 0; i < m_brain->GetCount(); ++i)
	{
		m_layers.PushBack(new ndLayersVariables((*m_brain)[i]));
	}
}

ndBrainTrainer::~ndBrainTrainer()
{
	ndAssert(0);
	for (ndInt32 i = 0; i < m_brain->GetCount(); ++i)
	{
		delete (m_layers[i]);
	}

}

ndBrain* ndBrainTrainer::GetBrain() const
{
	return m_brain;
}

ndBrainTrainer::ndSolveModel ndBrainTrainer::GetModel() const
{
	return m_model;
}

void ndBrainTrainer::SetModel(ndSolveModel model)
{
	m_model = model;
}

ndReal ndBrainTrainer::GetRegularizer() const
{
	return m_weighDecayRegularizer;
}

void ndBrainTrainer::SetRegularizer(ndReal regularizer)
{
	m_weighDecayRegularizer = ndClamp(regularizer, ndReal(0.0f), ndReal(0.01f));
}

//void ndBrainTrainer::PrefixScan()
//{
//	const ndArray<ndBrainLayer*>& layers = *m_brain;
//	m_inputOutputPrefixScan.SetCount(0);
//	m_inputOutputPrefixScan.PushBack((layers[0]->GetInputSize() + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT);
//	for (ndInt32 i = 0; i < layers.GetCount(); ++i)
//	{
//		ndBrainLayer* const layer = layers[i];
//		ndInt32 size = (layer->GetOutputSize() + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
//		m_inputOutputPrefixScan.PushBack(size);
//	}
//	ndInt32 size = 0;
//	for (ndInt32 i = 0; i < m_inputOutputPrefixScan.GetCount(); ++i)
//	{
//		ndInt32 count = m_inputOutputPrefixScan[i];
//		m_inputOutputPrefixScan[i] = size;
//		size += count;
//	}
//	m_inputOutputPrefixScan.PushBack(size);
//	
//	m_z.SetCount(size);
//	m_zDerivative.SetCount(size);
//	m_biasGradients.SetCount(size);
//	m_biasGradient_u.SetCount(size);
//	m_biasGradient_v.SetCount(size);
//	m_biasGradientsAcc.SetCount(size);
//	
//	m_z.Set(ndReal (0.0f));
//	m_zDerivative.Set(ndReal(0.0f));
//	m_biasGradients.Set(ndReal(0.0f));
//	m_biasGradient_u.Set(ndReal (0.0f));
//	m_biasGradient_v.Set(ndReal (0.0f));
//	m_biasGradientsAcc.Set(ndReal(0.0f));
//	
//	m_weightGradientsPrefixScan.SetCount(0);
//	for (ndInt32 i = 0; i < layers.GetCount(); ++i)
//	{
//		ndBrainLayer* const layer = layers[i];
//		ndInt32 stride = (layer->GetInputSize() + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
//		m_weightGradientsPrefixScan.PushBack (stride * layer->GetOutputSize());
//	}
//	
//	ndInt32 sum = 0;
//	for (ndInt32 i = 0; i < m_weightGradientsPrefixScan.GetCount(); ++i)
//	{
//		ndInt32 count = m_weightGradientsPrefixScan[i];
//		m_weightGradientsPrefixScan[i] = sum;
//		sum += count;
//	}
//	m_weightGradientsPrefixScan.PushBack(sum);
//	m_weightGradients.SetCount(sum);
//	m_weightGradient_u.SetCount(sum);
//	m_weightGradient_v.SetCount(sum);
//	
//	m_weightGradients.Set(0.0f);
//	m_weightGradient_u.Set(0.0f);
//	m_weightGradient_v.Set(0.0f);
//}

void ndBrainTrainer::AcculumateGradients(const ndBrainTrainer& src, ndInt32 threadIndex, ndInt32 threadCount)
{
	ndStartEnd biasStartEnd(m_biasGradientsAcc.GetCount(), threadIndex, threadCount);
	ndBrainMemVector bias0(&m_biasGradientsAcc[biasStartEnd.m_start], biasStartEnd.m_end - biasStartEnd.m_start);
	const ndBrainMemVector bias1(&src.m_biasGradientsAcc[biasStartEnd.m_start], biasStartEnd.m_end - biasStartEnd.m_start);
	bias0.Add(bias1);

	ndStartEnd weightStartEnd(m_weightGradients.GetCount(), threadIndex, threadCount);
	ndBrainMemVector weigh0(&m_weightGradients[weightStartEnd.m_start], weightStartEnd.m_end - weightStartEnd.m_start);
	const ndBrainMemVector weigh1(&src.m_weightGradients[weightStartEnd.m_start], weightStartEnd.m_end - weightStartEnd.m_start);
	weigh0.Add(weigh1);
}


void ndBrainTrainer::BackPropagateCalculateWeightsGradient(ndInt32 layerIndex)
{
	ndAssert(0);
	//const ndArray<ndBrainLayer*>& layers = *m_brain;
	//ndBrainLayer* const layer = layers[layerIndex];
	//
	//const ndBrainMemVector biasGradients(&m_biasGradients[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetOutputSize());
	//
	//const ndInt32 inputCount = layer->GetInputSize();
	//const ndInt32 stride = (inputCount + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
	//ndReal* weightGradientPtr = &m_weightGradients[m_weightGradientsPrefixScan[layerIndex]];
	//
	//const ndBrainMemVector z0(&m_z[m_inputOutputPrefixScan[layerIndex]], inputCount);
	//for (ndInt32 i = 0; i < layer->GetOutputSize(); ++i)
	//{
	//	ndBrainMemVector weightGradient(weightGradientPtr, inputCount);
	//	ndReal gValue = biasGradients[i];
	//	weightGradient.ScaleAdd(z0, gValue);
	//	weightGradient.FlushToZero();
	//	weightGradientPtr += stride;
	//}
}


void ndBrainTrainer::UpdateWeights(ndReal learnRate, ndInt32 batchSize)
{
	ndReal weight = 1.0f / ndReal(batchSize);
	m_biasGradientsAcc.Scale(weight);
	m_weightGradients.Scale(weight);
	if (m_model == m_adam)
	{
		AdamUpdate(learnRate);
	}
	else
	{
		StochasticUpdate(learnRate);
	}
	
}

void ndBrainTrainer::StochasticUpdate(ndReal learnRate)
{
	learnRate *= ndReal(-1.0f);
	ndReal regularizer = -GetRegularizer();
	const ndArray<ndBrainLayer*>& layers = *m_brain;
	for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
	{
		ndAssert(0);
		//ndBrainLayer* const layer = layers[i];
		//const ndInt32 inputSize = layer->GetInputSize();
		//const ndInt32 outputSize = layer->GetOutputSize();
		//
		//ndBrainVector& bias = layer->GetBias();
		//ndBrainMemVector biasGradients(&m_biasGradientsAcc[m_inputOutputPrefixScan[i + 1]], outputSize);
		//biasGradients.Scale(learnRate);
		//biasGradients.ScaleAdd(bias, regularizer);
		//bias.Add(biasGradients);
		//bias.FlushToZero();
		//
		//const ndInt32 weightGradientStride = (inputSize + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
		//const ndReal* weightGradientPtr = &m_weightGradients[m_weightGradientsPrefixScan[i]];
		//
		//ndBrainMatrix& weightMatrix = layer->m_weights;
		//for (ndInt32 j = 0; j < outputSize; ++j)
		//{
		//	ndBrainVector& weightVector = weightMatrix[j];
		//	ndBrainMemVector weightGradients(weightGradientPtr, inputSize);
		//	weightGradients.Scale(learnRate);
		//	weightGradients.ScaleAdd(weightVector, regularizer);
		//	weightVector.Add(weightGradients);
		//	weightVector.FlushToZero();
		//	weightGradientPtr += weightGradientStride;
		//}
	}
}

void ndBrainTrainer::AdamUpdate(ndReal learnRate)
{
	ndReal betaWeight = ndReal(1.0f) / (ndReal(1.0f) - m_betaAcc);
	ndReal alphaWeight = ndReal(1.0f) / (ndReal(1.0f) - m_alphaAcc);

	ndInt32 tempSize = (ndMax(m_biasGradientsAcc.GetCount(), m_weightGradients.GetCount()) + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
	ndReal* const tempBuff = ndAlloca(ndReal, tempSize * 2);
	{
		m_biasGradient_v.Scale(m_beta);
		m_biasGradient_u.Scale(m_alpha);

		ndBrainMemVector biasGradientsAcc2(tempBuff, m_biasGradientsAcc.GetCount());
		biasGradientsAcc2.Set(m_biasGradientsAcc);
		biasGradientsAcc2.Mul(m_biasGradientsAcc);
		m_biasGradient_v.ScaleAdd(biasGradientsAcc2, ndReal (1.0f) - m_beta);
		m_biasGradient_u.ScaleAdd(m_biasGradientsAcc, ndReal(1.0f) - m_alpha);

		if (m_betaAcc > ndReal(0.0f))
		{
			ndBrainMemVector uHat(tempBuff, m_biasGradientsAcc.GetCount());
			ndBrainMemVector vHat(tempBuff + tempSize, m_biasGradientsAcc.GetCount());
			uHat.Set(m_biasGradient_u);
			vHat.Set(m_biasGradient_v);
			vHat.Scale(betaWeight);
			uHat.Scale(alphaWeight);
			for (ndInt32 i = m_biasGradientsAcc.GetCount() - 1; i >= 0; --i)
			{
				ndReal bias_den = ndReal(1.0f) / (ndReal(ndSqrt(vHat[i])) + m_epsilon);
				m_biasGradientsAcc[i] = uHat[i] * bias_den;
			}
		}
		else
		{
			for (ndInt32 i = m_biasGradientsAcc.GetCount() - 1; i >= 0; --i)
			{
				ndReal bias_den = ndReal(1.0f) / (ndReal(ndSqrt(m_biasGradient_v[i])) + m_epsilon);
				m_biasGradientsAcc[i] = m_biasGradient_u[i] * bias_den;
			}
		}
	}

	{
		m_weightGradient_v.Scale(m_beta);
		m_weightGradient_u.Scale(m_alpha);
		
		ndBrainMemVector weightGradients2(tempBuff, m_weightGradients.GetCount());
		weightGradients2.Set(m_weightGradients);
		weightGradients2.Mul(m_weightGradients);
		m_weightGradient_v.ScaleAdd(weightGradients2, ndReal(1.0f) - m_beta);
		m_weightGradient_u.ScaleAdd(m_weightGradients, ndReal(1.0f) - m_alpha);
		
		if (m_betaAcc > ndReal(0.0f))
		{
			ndBrainMemVector uHat(tempBuff, m_weightGradients.GetCount());
			ndBrainMemVector vHat(tempBuff + tempSize, m_weightGradients.GetCount());
			uHat.Set(m_weightGradient_u);
			vHat.Set(m_weightGradient_v);
			vHat.Scale(betaWeight);
			uHat.Scale(alphaWeight);
			 
			for (ndInt32 i = m_weightGradients.GetCount() - 1; i >= 0; --i)
			{
				ndReal bias_den = ndReal(1.0f) / (ndReal(ndSqrt(vHat[i])) + m_epsilon);
				m_weightGradients[i] = uHat[i] * bias_den;
			}
		}
		else
		{
			for (ndInt32 i = m_weightGradients.GetCount() - 1; i >= 0; --i)
			{
				ndReal bias_den = ndReal(1.0f) / (ndReal(ndSqrt(m_weightGradient_v[i])) + m_epsilon);
				m_weightGradients[i] = m_weightGradient_u[i] * bias_den;
			}
		}
	}

	m_betaAcc = ndFlushToZero(m_betaAcc * m_beta);
	m_alphaAcc = ndFlushToZero(m_alphaAcc * m_alpha);
	if (m_betaAcc < ndReal(1.0e-6f))
	{
		m_betaAcc = ndReal(0.0f);
	}
	StochasticUpdate(learnRate);
}

void ndBrainTrainer::ClearGradientsAcc()
{
	for (ndInt32 i = m_layers.GetCount() - 1; i >= 0; --i)
	{
		m_layers[i]->ClearGradAcc();
	}
}

void ndBrainTrainer::BackPropagateOutputLayer(const ndBrainVector& loss)
{
	ndAssert(0);
	////const ndArray<ndBrainLayer*>& layers = *m_brain;
	////const ndInt32 layerIndex = layers.GetCount() - 1;
	//
	////ndBrainLayer* const ouputLayer = layers[layerIndex];
	////const ndInt32 inputCount = ouputLayer->GetInputSize();
	////const ndInt32 outputCount = ouputLayer->GetOutputSize();
	//
	////ndBrainMemVector biasGradients(&m_biasGradients[m_inputOutputPrefixScan[layerIndex + 1]], outputCount);
	////ndBrainMemVector biasGradientsAcc(&m_biasGradientsAcc[m_inputOutputPrefixScan[layerIndex + 1]], outputCount);
	////const ndBrainMemVector z(&m_z[m_inputOutputPrefixScan[layerIndex + 1]], outputCount);
	////const ndBrainMemVector zDerivative(&m_zDerivative[m_inputOutputPrefixScan[layerIndex + 1]], outputCount);
	////
	////loss.GetLoss(z, biasGradients);
	//ndLayersVariables& outputLayer = *m_layers[m_layers.GetCount() - 1];
	//
	//ndReal* const activationDerivativeBuffer = ndAlloca(ndReal, outputLayer.m_z.GetCount() * 2);
	//ndBrainMemVector activationDerivative(activationDerivativeBuffer, outputLayer.m_z.GetCount());
	//outputLayer.m_layer->ActivationDerivative(outputLayer.m_z, activationDerivative);
	//activationDerivative.Mul(loss);
	//
	////biasGradients.Mul(zDerivative);
	////biasGradientsAcc.Add(biasGradients);
	////biasGradientsAcc.FlushToZero();
	////
	////const ndInt32 stride = (inputCount + D_DEEP_BRAIN_DATA_ALIGMENT - 1) & -D_DEEP_BRAIN_DATA_ALIGMENT;
	////ndReal* weightGradientPtr = &m_weightGradients[m_weightGradientsPrefixScan[layerIndex]];
	////const ndBrainMemVector z0(&m_z[m_inputOutputPrefixScan[layerIndex]], inputCount);
	////for (ndInt32 i = 0; i < outputCount; ++i)
	////{
	////	ndBrainMemVector weightGradient(weightGradientPtr, inputCount);
	////	ndReal gValue = biasGradients[i];
	////	weightGradient.ScaleAdd(z0, gValue);
	////	weightGradient.FlushToZero();
	////	weightGradientPtr += stride;
	////}
	//
	//ndAssert(m_layers.GetCount() > 2);
	//ndLayersVariables& linealLayer = *m_layers[m_layers.GetCount() - 2];
	//const ndBrainVector& input = m_layers[m_layers.GetCount() - 3]->m_z;
	//linealLayer.m_layer->CalculateOutputLayersParamGradients(input, activationDerivative, linealLayer.m_gradBias, linealLayer.m_gradWeight);
}

void ndBrainTrainer::BackPropagateCalculateBiasGradient(ndInt32 layerIndex)
{
	ndAssert(0);
	//const ndArray<ndBrainLayer*>& layers = *m_brain;
	//ndBrainLayer* const layer = layers[layerIndex + 1];
	//
	//const ndBrainMemVector biasGradients1(&m_biasGradients[m_inputOutputPrefixScan[layerIndex + 2]], layer->GetOutputSize());
	//ndBrainMemVector biasGradients(&m_biasGradients[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetInputSize());
	//ndBrainMemVector biasGradientsAcc(&m_biasGradientsAcc[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetInputSize());
	//const ndBrainMemVector zDerivative(&m_zDerivative[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetInputSize());

	//layer->m_weights.TransposeMul(biasGradients1, biasGradients);
	//biasGradients.Mul(zDerivative);
	//biasGradientsAcc.Add(biasGradients);
	//biasGradientsAcc.FlushToZero();
}


void ndBrainTrainer::BackPropagate(const ndBrainVector& input, ndBrainLoss& loss)
{
#if 0
	m_layers[0]->m_layer->MakePrediction(input, m_layers[0]->m_z);
	ndInt32 maxOutpuSize = m_layers[0]->m_layer->GetOutputSize();
	for (ndInt32 i = 1; i < m_layers.GetCount(); ++i)
	{
		ndLayersVariables& layer = *m_layers[i];
		layer.m_layer->MakePrediction(m_layers[i - 1]->m_z, m_layers[i]->m_z);
		maxOutpuSize = ndMax(maxOutpuSize, layer.m_layer->GetOutputSize());
	}

ndBrainVector xxxx0;
ndBrainVector xxxx1;
xxxx0.SetCount(input.GetCount());
xxxx1.SetCount(m_brain->GetOutputSize());
xxxx1.Set(ndReal(1.0f));
//m_brain->CalculateInputGradient(input, xxxx1, xxxx0);
m_brain->CalculateInputGradient(input, xxxx0);
ndAssert(0);
	ndReal* const lossBuffer = ndAlloca(ndReal, m_brain->GetOutputSize() + 256);
	ndBrainMemVector outputLost(lossBuffer, m_brain->GetOutputSize());

	const ndBrainVector& output = m_layers[m_layers.GetCount() - 1]->m_z;
	loss.GetLoss(output, outputLost);

	//for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
	//{
	//	ndBrainLayer* const layer = layers[i];
	//	const ndBrainMemVector z(&m_z[m_inputOutputPrefixScan[i + 1]], layer->GetOutputSize());
	//	ndBrainMemVector zDerivative(&m_zDerivative[m_inputOutputPrefixScan[i + 1]], layer->GetOutputSize());
	//	layer->ActivationDerivative(z, zDerivative);
	//}

	BackPropagateOutputLayer(outputLost);

	//for (ndInt32 i = m_layers.GetCount() - 2; i >= 1; --i)
	//{
	//	BackPropagateCalculateBiasGradient(i);
	//	BackPropagateCalculateWeightsGradient(i);
	//}

	ndLayersVariables& startLayer = *m_layers[m_layers.GetCount() - 3];
	ndReal* const activationDerivativeBuffer = ndAlloca(ndReal, maxOutpuSize * 2);
	ndBrainMemVector activationDerivative(activationDerivativeBuffer, startLayer.m_z.GetCount());
	startLayer.m_layer->ActivationDerivative(startLayer.m_z, activationDerivative);

	for (ndInt32 i = m_layers.GetCount() - 3; i >= 2; i -= 2)
	{
		//const ndArray<ndBrainLayer*>& layers = *m_brain;
		//ndBrainLayer* const layer = layers[layerIndex + 1];
		//
		//const ndBrainMemVector biasGradients1(&m_biasGradients[m_inputOutputPrefixScan[layerIndex + 2]], layer->GetOutputSize());
		//ndBrainMemVector biasGradients(&m_biasGradients[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetInputSize());
		//ndBrainMemVector biasGradientsAcc(&m_biasGradientsAcc[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetInputSize());
		//const ndBrainMemVector zDerivative(&m_zDerivative[m_inputOutputPrefixScan[layerIndex + 1]], layer->GetInputSize());

		//layer->m_weights.TransposeMul(biasGradients1, biasGradients);
		//biasGradients.Mul(zDerivative);
		//biasGradientsAcc.Add(biasGradients);
		//biasGradientsAcc.FlushToZero();

		ndLayersVariables& layer = *m_layers[i];

	}
	ndAssert(0);
#else

	ndFixSizeArray<ndInt32, 256> prefixScan;
	const ndArray<ndBrainLayer*>& layers = *m_brain;

	prefixScan.PushBack(0);
	ndInt32 maxSize = 0;
	ndInt32 sizeAcc = (layers[0]->GetInputSize() + 7) & -8;
	for (ndInt32 i = 0; i < m_brain->GetCount(); ++i)
	{
		prefixScan.PushBack(sizeAcc);
		sizeAcc += (layers[i]->GetInputSize() + 7) & -8;
		maxSize = ndMax(maxSize, layers[i]->GetOutputSize());
	}
	prefixScan.PushBack(sizeAcc);

	const ndReal* const memBuffer = ndAlloca(ndReal, sizeAcc + 8);
	const ndReal* const gradientBuffer = ndAlloca(ndReal, maxSize * 2 + 256);

	ndBrainMemVector in0(memBuffer, input.GetCount());
	in0.Set(input);
	for (ndInt32 i = 0; i < m_brain->GetCount(); ++i)
	{
		ndBrainMemVector in(memBuffer + prefixScan[i + 0], layers[i]->GetInputSize());
		ndBrainMemVector out(memBuffer + prefixScan[i + 1], layers[i]->GetOutputSize());
		layers[i]->MakePrediction(in, out);
	}
	const ndBrainMemVector output(memBuffer + prefixScan[layers.GetCount()], m_brain->GetOutputSize());

ndBrainVector xxxx0;
xxxx0.SetCount(input.GetCount());
m_brain->CalculateInputGradient(input, xxxx0);

	ndBrainMemVector gradientIn(gradientBuffer, m_brain->GetOutputSize());
	ndBrainMemVector gradientOut(gradientBuffer + maxSize + 128, m_brain->GetOutputSize());
	loss.GetLoss(output, gradientOut);

ndAssert(0);
	for (ndInt32 i = layers.GetCount() - 1; i >= 0; --i)
	{
		const ndBrainLayer* const layer = layers[i];
		gradientIn.SetSize(layer->GetInputSize());
		const ndBrainMemVector out(memBuffer + prefixScan[i + 1], layer->GetOutputSize());
		layer->InputDerivative(out, gradientOut, gradientIn);
		gradientIn.Swap(gradientOut);
	}
	//inputGradients.Set(gradientOut);
#endif
}