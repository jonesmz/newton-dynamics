/* Copyright (c) <2003-2022> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "ndSandboxStdafx.h"
#include "ndSkyBox.h"
#include "ndUIEntity.h"
#include "ndDemoMesh.h"
#include "ndDemoCamera.h"
#include "ndPhysicsUtils.h"
#include "ndPhysicsWorld.h"
#include "ndMakeStaticMap.h"
#include "ndDemoEntityManager.h"
#include "ndDemoInstanceEntity.h"

// this is an implementation of the vanilla dqn agent trainer as descrived 
// on the nature paper below. 
// https://storage.googleapis.com/deepmind-media/dqn/DQNNaturePaper.pdf

namespace ndController_0
{
	#define D_REPLAY_BASH_SIZE		(32)
	#define D_REPLAY_BUFFERSIZE		(1024 * 64)
	#define D_DISCOUNT_FACTOR		ndReal (0.99f)
	#define D_EPSILON_GREEDY		ndReal (1.0e-4f)
	#define D_MIN_EXPLARE_FACTOR	ndReal (0.01f)

	#define D_PUSH_FORCE ndFloat32 (20.0f)

	enum ndActionSpace
	{
		m_statePut,
		m_pushLeft,
		m_pushRight,
		m_acctionsCount
	};

	enum ndStateSpace
	{
		m_cartPosition,
		m_cartVelocity,
		m_poleAngle,
		m_poleOmega,
		m_stateCount
	};

	class ndCartpoleBase : public ndModelArticulation
	{
		public:
		virtual void ResetModel() const = 0;
		virtual bool IsTerminal() const = 0;
		virtual ndReal GetReward() const = 0;
		virtual ndInt32 GetAction(ndReal greedy) const = 0;
		virtual void GetObservation(ndReal* const state) const = 0;
	};

	class ndQValuePredictor : public ndBrain
	{
		public:
		ndQValuePredictor()
			:ndBrain()
		{
			ndBrainLayer* const inputLayer = new ndBrainLayer(m_stateCount, 128, m_relu);
			ndBrainLayer* const hiddenLayer0 = new ndBrainLayer(inputLayer->GetOuputSize(), 128, m_relu);
			ndBrainLayer* const hiddenLayer1 = new ndBrainLayer(hiddenLayer0->GetOuputSize(), 128, m_relu);
			ndBrainLayer* const ouputLayer = new ndBrainLayer(hiddenLayer1->GetOuputSize(), m_acctionsCount, m_lineal);

			BeginAddLayer();
			AddLayer(inputLayer);
			AddLayer(hiddenLayer0);
			AddLayer(hiddenLayer1);
			AddLayer(ouputLayer);
			EndAddLayer();
			InitGaussianWeights(0.0f, 0.125f);
		}
	};

	class ndDQNAgent
	{
		public:
		ndDQNAgent(ndCartpoleBase* const model)
			:m_onlineNetwork()
			,m_targetNetwork(m_onlineNetwork)
			,m_input()
			,m_output()
			,m_trainer(&m_onlineNetwork)
			,m_targetInstance(&m_targetNetwork)
			,m_shuffleBuffer(D_REPLAY_BUFFERSIZE)
			,m_replayBuffer(D_REPLAY_BUFFERSIZE)
			,m_currentTransition()
			,m_model(model)
			,m_gamma(D_DISCOUNT_FACTOR)
			,m_epsilonGreedy(1.0f)
			,m_frameCount(0)
		{
			m_shuffleBuffer.SetCount(0);
			m_input.SetCount(m_stateCount);
			m_output.SetCount(m_acctionsCount);
		}

		void BackPropagate()
		{
			class ndTestValidator : public ndBrainTrainer::ndValidation
			{
				public:
				ndTestValidator(ndBrainTrainer& trainer)
					:ndBrainTrainer::ndValidation(trainer)
					,m_minError(1.0e10f)
					,m_step(0)
					,m_step0(0)
				{
				}

				ndReal Validate(const ndBrainMatrix& inputBatch, const ndBrainMatrix& groundTruth)
				{
					//ndReal error = ndBrainTrainer::ndValidation::Validate(inputBatch, groundTruth);
					//if (error < m_minError)
					//{
					//	m_minError = error;
					//	ndExpandTraceMessage("%f; %d; %d\n", m_minError, m_step, m_step - m_step0);
					//	m_step0 = m_step;
					//}
					//m_step++;
					////ndExpandTraceMessage("%f\n", error);
					//return error;
					return 0;
				}
				ndReal m_minError;
				ndInt32 m_step;
				ndInt32 m_step0;
			};

			
			ndBrainMatrix inputBatch(D_REPLAY_BASH_SIZE, m_stateCount);
			ndBrainMatrix groundTruth(D_REPLAY_BASH_SIZE, m_acctionsCount);
			ndTestValidator validator(m_trainer);

			ndBrainInstance& instance = m_trainer.GetInstance();

			m_shuffleBuffer.RandomShuffle(m_shuffleBuffer.GetCount());
			for (ndInt32 i = 0; i < D_REPLAY_BASH_SIZE; ++i)
			{
				ndInt32 index = m_shuffleBuffer[i];
				const ndBrainReplayTransitionMemory<ndInt32, m_stateCount>& transition = m_replayBuffer[index];

				for (ndInt32 j = 0; j < m_stateCount; ++j)
				{
					m_input[j] = transition.m_state[j];
				}
				instance.MakePrediction(m_input, m_output);
				for (ndInt32 j = 0; j < m_acctionsCount; ++j)
				{
					groundTruth[i][j] = m_output[j];
				}

				for (ndInt32 j = 0; j < m_stateCount; ++j)
				{
					m_input[j] = transition.m_nextState[j];
				}
				m_targetInstance.MakePrediction(m_input, m_output);
				for (ndInt32 j = 0; j < m_acctionsCount; ++j)
				{
					groundTruth[i][j] = m_output[j];
				}
				//Optimize(ndValidation & validator, const ndBrainMatrix & inputBatch, const ndBrainMatrix & groundTruth, ndReal learnRate, ndInt32 steps);
			}
		}

		void Train()
		{
			m_model->GetObservation(&m_currentTransition.m_nextState[0]);
			m_currentTransition.m_reward = m_model->GetReward();
			m_currentTransition.m_terminalState = m_model->IsTerminal();
			m_replayBuffer.AddTransition(m_currentTransition);
			if (m_currentTransition.m_terminalState)
			{
				m_model->ResetModel();
			}

			if (m_frameCount < D_REPLAY_BUFFERSIZE)
			{
				m_shuffleBuffer.PushBack(m_frameCount);
			}

			m_currentTransition.m_state = m_currentTransition.m_nextState;
			m_currentTransition.m_action[0] = m_model->GetAction(m_epsilonGreedy);

			if (m_frameCount % (D_REPLAY_BASH_SIZE * 4) == (D_REPLAY_BASH_SIZE * 4 - 1))
			{
				m_epsilonGreedy = ndMax(m_epsilonGreedy - D_EPSILON_GREEDY, D_MIN_EXPLARE_FACTOR);
			}

			if (m_frameCount > (D_REPLAY_BASH_SIZE * 8))
			{
				// do back propagation on the 
				BackPropagate();
			}

			if ((m_frameCount % 1000) == (1000 - 1))
			{
				// update on line network
				m_targetNetwork.CopyFrom(m_onlineNetwork);
			}

			m_frameCount++;
		}

		ndInt32 GetMaxValueAction() const
		{
			for (ndInt32 i = 0; i < m_stateCount; ++i)
			{
				m_input[i] = m_currentTransition.m_state[i];
			}
			ndBrainInstance& instance = m_trainer.GetInstance();
			instance.MakePrediction(m_input, m_output);

			ndInt32 action = 0;
			ndReal maxReward = m_output[0];
			for (ndInt32 i = 1; i < m_acctionsCount; ++i)
			{
				if (m_output[i] > maxReward)
				{
					action = i;
					maxReward = m_output[i];
				}
			}
			return action;
		}

		ndQValuePredictor m_onlineNetwork;
		ndQValuePredictor m_targetNetwork;
		mutable ndBrainVector m_input;
		mutable ndBrainVector m_output;
		mutable ndBrainTrainer m_trainer;
		mutable ndBrainInstance m_targetInstance;
		ndArray<ndInt32> m_shuffleBuffer;
		ndBrainReplayBuffer<ndInt32, m_stateCount> m_replayBuffer;
		ndBrainReplayTransitionMemory<ndInt32, m_stateCount> m_currentTransition;
		ndCartpoleBase* m_model;

		ndReal m_gamma;
		ndReal m_epsilonGreedy;
		ndInt32 m_frameCount;
	};

	class ndCartpole : public ndCartpoleBase
	{
		public:
		ndCartpole()
			:ndCartpoleBase()
			,m_cartMatrix(ndGetIdentityMatrix())
			,m_poleMatrix(ndGetIdentityMatrix())
			,m_cart(nullptr)
			,m_pole(nullptr)
			,m_agent(nullptr)
		{
		}

		virtual bool IsTerminal() const
		{
			const ndMatrix& matrix = m_pole->GetMatrix();
			bool fail = (matrix.m_front.m_y) < ndFloat32(0.96f);
			fail = fail || (matrix.m_posit.m_x > ndFloat32(4.0f));
			fail = fail || (matrix.m_posit.m_x < ndFloat32(-4.0f));
			return fail;
		}

		virtual ndReal GetReward() const
		{
			return ndReal(m_pole->GetMatrix().m_up.m_y);
		}

		virtual ndInt32 GetAction(ndReal greedy) const
		{
			ndInt32 action = 0;
			ndFloat32 explore = ndRand();
			if (explore <= greedy)
			{
				action = ndInt32(ndRandInt() % m_acctionsCount);
			}
			else
			{
				action = m_agent->GetMaxValueAction();
			}

			return action;
		}

		void GetObservation(ndReal* const state) const
		{
			ndFloat32 posit = m_cart->GetMatrix().m_posit.m_x;
			ndFloat32 veloc = m_cart->GetVelocity().m_x;

			ndFloat32 angle = ndAsin(m_pole->GetMatrix().m_up.m_y);
			ndFloat32 omega = m_pole->GetOmega().m_z;

			state[m_poleAngle] = ndReal(angle);
			state[m_poleOmega] = ndReal(omega);
			state[m_cartPosition] = ndReal(posit);
			state[m_cartVelocity] = ndReal(veloc);
		}

		virtual void ResetModel() const
		{
			m_cart->SetOmega(ndVector::m_zero);
			m_cart->SetVelocity(ndVector::m_zero);
			m_pole->SetOmega(ndVector::m_zero);
			m_pole->SetVelocity(ndVector::m_zero);
			m_cart->SetMatrix(m_cartMatrix);
			m_pole->SetMatrix(m_poleMatrix);
		}

		void Update(ndWorld* const world, ndFloat32 timestep)
		{
			ndModelArticulation::Update(world, timestep);

			ndVector force(m_pole->GetForce());
			ndInt32 action = m_agent->m_currentTransition.m_action[0];
			if (action == m_pushLeft)
			{
				force.m_x = -D_PUSH_FORCE;
			}
			else if (action == m_pushRight)
			{
				force.m_x = D_PUSH_FORCE;
			}
			m_pole->SetForce(force);
		}

		void PostUpdate(ndWorld* const world, ndFloat32 timestep)
		{
			ndModelArticulation::PostUpdate(world, timestep);
			m_agent->Train();
		}

		ndMatrix m_cartMatrix;
		ndMatrix m_poleMatrix;
		ndBodyDynamic* m_cart;
		ndBodyDynamic* m_pole;
		ndSharedPtr<ndDQNAgent> m_agent;
	};

	void BuildModel(ndCartpole* const model, ndDemoEntityManager* const scene, const ndMatrix& location)
	{
		ndFloat32 xSize = 0.25f;
		ndFloat32 ySize = 0.125f;
		ndFloat32 zSize = 0.15f;
		ndFloat32 cartMass = 5.0f;
		ndFloat32 poleMass = 1.0f;
		ndPhysicsWorld* const world = scene->GetWorld();
		
		// add hip body
		ndSharedPtr<ndBody> cartBody(world->GetBody(AddBox(scene, location, cartMass, xSize, ySize, zSize, "smilli.tga")));
		ndModelArticulation::ndNode* const modelRoot = model->AddRootBody(cartBody);

		ndMatrix matrix(cartBody->GetMatrix());
		matrix.m_posit.m_y += ySize / 2.0f;

		// make single leg
		ndFloat32 poleLength = 0.4f;
		ndFloat32 poleRadio = 0.025f;
		
		ndSharedPtr<ndBody> poleBody(world->GetBody(AddCapsule(scene, ndGetIdentityMatrix(), poleMass, poleRadio, poleRadio, poleLength, "smilli.tga")));
		ndMatrix poleLocation(ndRollMatrix(90.0f * ndDegreeToRad) * matrix);
		poleLocation.m_posit.m_y += poleLength * 0.5f;
		poleBody->SetMatrix(poleLocation);

		ndMatrix polePivot(ndYawMatrix(90.0f * ndDegreeToRad) * poleLocation);
		polePivot.m_posit.m_y -= poleLength * 0.5f;
		ndSharedPtr<ndJointBilateralConstraint> poleJoint(new ndJointHinge(polePivot, poleBody->GetAsBodyKinematic(), modelRoot->m_body->GetAsBodyKinematic()));

		world->AddJoint(poleJoint);

		// add model limbs
		model->AddLimb(modelRoot, poleBody, poleJoint);

		ndSharedPtr<ndJointBilateralConstraint> fixJoint(new ndJointSlider(cartBody->GetMatrix(), cartBody->GetAsBodyDynamic(), world->GetSentinelBody()));
		world->AddJoint(fixJoint);

		model->m_cart = cartBody->GetAsBodyDynamic();
		model->m_pole = poleBody->GetAsBodyDynamic();
		model->m_cartMatrix = cartBody->GetMatrix();
		model->m_poleMatrix = poleBody->GetMatrix();

		model->m_agent = ndSharedPtr<ndDQNAgent>(new ndDQNAgent(model));
	}

	ndModelArticulation* CreateModel(ndDemoEntityManager* const scene, const ndMatrix& location)
	{
		ndCartpole* const model = new ndCartpole();
		BuildModel(model, scene, location);
		return model;
	}
}

using namespace ndController_0;
void ndCartpoleController(ndDemoEntityManager* const scene)
{
	// build a floor
	//BuildFloorBox(scene, ndGetIdentityMatrix());
	BuildFlatPlane(scene, true);

	ndSetRandSeed(42);
	ndWorld* const world = scene->GetWorld();
	ndMatrix matrix(ndYawMatrix(-0.0f * ndDegreeToRad));

	ndSharedPtr<ndModel> model(CreateModel(scene, matrix));
	world->AddModel(model);
	
	matrix.m_posit.m_x -= 0.0f;
	matrix.m_posit.m_y += 0.5f;
	matrix.m_posit.m_z += 2.0f;
	ndQuaternion rotation(ndVector(0.0f, 1.0f, 0.0f, 0.0f), 90.0f * ndDegreeToRad);
	scene->SetCameraMatrix(rotation, matrix.m_posit);
}