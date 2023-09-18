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

namespace ndController_1
{
	#define USE_TD3
	#define ND_TRAIN_MODEL

	#define ND_MAX_WHEEL_TORQUE		(ndFloat32 (10.0f))
	#define ND_MAX_LEG_ANGLE_STEP	(ndFloat32 (4.0f) * ndDegreeToRad)
	#define ND_MAX_LEG_JOINT_ANGLE	(ndFloat32 (30.0f) * ndDegreeToRad)

	enum ndActionSpace
	{
		m_softLegControl,
		m_softWheelControl,
		m_actionsSize
	};

	enum ndStateSpace
	{
		m_topBoxAngle,
		m_topBoxOmega,
		m_jointAngle,
		m_stateSize
	};

	class ndModelUnicycle : public ndModelArticulation
	{
		public:
		class ndBasePose
		{
			public:
			ndBasePose()
				:m_body(nullptr)
			{
			}

			ndBasePose(ndBodyDynamic* const body)
				:m_veloc(body->GetVelocity())
				,m_omega(body->GetOmega())
				,m_posit(body->GetPosition())
				,m_rotation(body->GetRotation())
				,m_body(body)
			{
			}

			void SetPose() const
			{
				m_body->SetMatrix(ndCalculateMatrix(m_rotation, m_posit));
				m_body->SetOmega(m_omega);
				m_body->SetVelocity(m_veloc);
			}

			ndVector m_veloc;
			ndVector m_omega;
			ndVector m_posit;
			ndQuaternion m_rotation;
			ndBodyDynamic* m_body;
		};

		// implement controller player
#ifdef USE_TD3
		class ndControllerAgent : public ndBrainAgentDDPG<m_stateSize, m_actionsSize>
		{
			public:
			ndControllerAgent(ndSharedPtr<ndBrain>& actor)
				:ndBrainAgentDDPG<m_stateSize, m_actionsSize>(actor)
				,m_model(nullptr)
			{
			}
#else
		class ndControllerAgent : public ndBrainAgentDDPG<m_stateSize, m_actionsSize>
		{
			public:
			ndControllerAgent(ndSharedPtr<ndBrain>& actor)
				:ndBrainAgentDDPG<m_stateSize, m_actionsSize>(actor)
				, m_model(nullptr)
			{
			}
#endif

			void SetModel(ndModelUnicycle* const model)
			{
				m_model = model;
			}

			void GetObservation(ndReal* const state) const
			{
				m_model->GetObservation(state);
			}

			virtual void ApplyActions(ndReal* const actions) const
			{
				if (!m_model->HasSupportContact())
				{
					for (ndInt32 i = 0; i < m_actionsSize; ++i)
					{
						actions[i] = ndReal(0.0f);
					}
				}

				m_model->ApplyActions(actions);
			}

			ndModelUnicycle* m_model;
		};

		// the table based approach is not really practical
		// try implement DDPN controller using neural networks. 
#ifdef USE_TD3
		class ndControllerAgent_trainer: public ndBrainAgentTD3_Trainer<m_stateSize, m_actionsSize>
		{
			public:
			ndControllerAgent_trainer(const HyperParameters& hyperParameters, ndSharedPtr<ndBrain>& actor, ndSharedPtr<ndBrain>& critic)
				:ndBrainAgentTD3_Trainer<m_stateSize, m_actionsSize>(hyperParameters, actor, critic)
#else
		class ndControllerAgent_trainer: public ndBrainAgentDDPG_Trainer<m_stateSize, m_actionsSize>
		{
			public:
			ndControllerAgent_trainer(const HyperParameters& hyperParameters, ndSharedPtr<ndBrain>& actor, ndSharedPtr<ndBrain>& critic)
				:ndBrainAgentDDPG_Trainer<m_stateSize, m_actionsSize>(hyperParameters, actor, critic)
#endif
				,m_model(nullptr)
				,m_maxGain(-1.0e10f)
				,m_maxFrames(4000)
				,m_stopTraining(2000000)
				,m_timer(0)
				,m_modelIsTrained(false)
				,m_averageQValue()
				,m_averageFramesPerEpisodes()
			{
				SetActionNoise(ndReal (0.15f));
				#ifdef USE_TD3
					m_outFile = fopen("traingPerf-TD3.csv", "wb");
					fprintf(m_outFile, "td3\n");
				#else
					m_outFile = fopen("traingPerf-SAC.csv", "wb");
					fprintf(m_outFile, "sac\n");
				#endif
				m_timer = ndGetTimeInMicroseconds();
				InitWeights();
			}

			~ndControllerAgent_trainer()
			{
				if (m_outFile)
				{
					fclose(m_outFile);
				}
			}

			void SetModel(ndModelUnicycle* const model)
			{
				m_model = model;
			}

			ndReal GetReward() const
			{
				return m_model->GetReward();
			}

			virtual void ApplyActions(ndReal* const actions) const
			{
				if (GetEpisodeFrames() >= 10000)
				{
					for (ndInt32 i = 0; i < m_actionsSize; ++i)
					{
						ndReal gaussianNoise = ndReal(ndGaussianRandom(ndFloat32(actions[i]), ndFloat32(1.0f)));
						ndReal clippiedNoisyAction = ndClamp(gaussianNoise, ndReal(-1.0f), ndReal(1.0f));
						actions[i] = clippiedNoisyAction;
					}
				}
				m_model->ApplyActions(actions);
			}

			void GetObservation(ndReal* const state) const
			{
				m_model->GetObservation(state);
			}

			bool IsTerminal() const
			{
				bool state = m_model->IsTerminal();
				if (!IsSampling())
				{
					if (GetEpisodeFrames() >= 15000)
					{
						state = true;
					}

					m_averageQValue.Update(GetCurrentValue());
					if (state)
					{
						m_averageFramesPerEpisodes.Update(ndReal(GetEpisodeFrames()));
					}
				}
				return state;
			}

			void ResetModel() const
			{
				m_model->ResetModel();
			}

			void OptimizeStep()
			{
				ndInt32 stopTraining = GetFramesCount();
				if (stopTraining <= m_stopTraining)
				{
					ndInt32 episodeCount = GetEposideCount();
					#ifdef USE_TD3
						ndBrainAgentTD3_Trainer::OptimizeStep();
					#else
						ndBrainAgentDDPG_Trainer::OptimizeStep();
					#endif

					episodeCount -= GetEposideCount();
					if (m_averageFramesPerEpisodes.GetAverage() >= ndFloat32 (m_maxFrames))
					{
						if (m_averageQValue.GetAverage() > m_maxGain)
						{
							char fileName[1024];
							ndGetWorkingFileName(GetName().GetStr(), fileName);

							SaveToFile(fileName);
							ndExpandTraceMessage("saving to file: %s\n", fileName);
							ndExpandTraceMessage("episode: %d\taverageFrames: %f\taverageValue %f\n\n", GetEposideCount(), m_averageFramesPerEpisodes.GetAverage(), m_averageQValue.GetAverage());
							m_maxGain = m_averageQValue.GetAverage();
						}
					}

					if (episodeCount && !IsSampling())
					{
						ndExpandTraceMessage("step: %d\treward: %g\tframes: %g\n", GetFramesCount(), m_averageQValue.GetAverage(), m_averageFramesPerEpisodes.GetAverage());
						if (m_outFile)
						{
							fprintf(m_outFile, "%g\n", m_averageQValue.GetAverage());
							fflush(m_outFile);
						}
					}

					if (stopTraining == m_stopTraining)
					{
						m_modelIsTrained = true;
						ndExpandTraceMessage("\n");
						ndExpandTraceMessage("training complete\n");
						ndUnsigned64 timer = ndGetTimeInMicroseconds() - m_timer;
						ndExpandTraceMessage("time training: %f\n", ndFloat32(ndFloat64(timer) * ndFloat32(1.0e-6f)));
					}
				}
				if (m_model->IsOutOfBounds())
				{
					m_model->TelePort();
				}
			}

			FILE* m_outFile;
			ndModelUnicycle* m_model;
			ndFloat32 m_maxGain;
			ndInt32 m_maxFrames;
			ndInt32 m_stopTraining;
			ndUnsigned64 m_timer;
			bool m_modelIsTrained;
			mutable ndMovingAverage<128> m_averageQValue;
			mutable ndMovingAverage<128> m_averageFramesPerEpisodes;
		};

		ndModelUnicycle(ndSharedPtr<ndBrainAgent> agent)
			:ndModelArticulation()
			,m_agent(agent)
			,m_legJoint(nullptr)
			,m_wheelJoint(nullptr)
		{
		}

		void GetObservation(ndReal* const state)
		{
			ndBodyKinematic* const body = GetRoot()->m_body->GetAsBodyKinematic();
			const ndVector omega (body->GetOmega());
			const ndMatrix& matrix = body->GetMatrix();
			ndFloat32 sinAngle = ndClamp(matrix.m_up.m_x, ndFloat32(-0.9f), ndFloat32(0.9f));
			ndFloat32 angle = ndAsin(sinAngle);

			state[m_topBoxAngle] = ndReal(angle);
			state[m_topBoxOmega] = ndReal(omega.m_z);
			state[m_jointAngle] = ndReal(m_legJoint->GetAngle() / ND_MAX_LEG_JOINT_ANGLE);
		}

		void ApplyActions(ndReal* const actions) const
		{
			ndFloat32 legAngle = actions[m_softLegControl] * ND_MAX_LEG_ANGLE_STEP + m_legJoint->GetAngle();
			legAngle = ndClamp (legAngle, -ND_MAX_LEG_JOINT_ANGLE, ND_MAX_LEG_JOINT_ANGLE);
			m_legJoint->SetTargetAngle(legAngle);

			ndBodyDynamic* const wheelBody = m_wheelJoint->GetBody0()->GetAsBodyDynamic();
			const ndMatrix matrix(m_wheelJoint->GetLocalMatrix1() * m_wheelJoint->GetBody1()->GetMatrix());

			ndVector torque(matrix.m_front.Scale(actions[m_softWheelControl] * ND_MAX_WHEEL_TORQUE));
			wheelBody->SetTorque(torque);
		}

		ndReal GetReward() const
		{
			if (IsTerminal())
			{
				return ndReal(0.0f);
			}

			ndFloat32 legReward = ndReal(ndPow(ndEXP, -ndFloat32(10000.0f) * m_legJoint->GetAngle() * m_legJoint->GetAngle()));

			ndBodyKinematic* const boxBody = GetRoot()->m_body->GetAsBodyKinematic();
			const ndMatrix& matrix = boxBody->GetMatrix();
			ndFloat32 sinAngle = matrix.m_up.m_x;
			ndFloat32 boxReward = ndReal(ndPow(ndEXP, -ndFloat32(1000.0f) * sinAngle * sinAngle));

			ndFloat32 reward = (boxReward + legReward) / 2.0f;
			return ndReal(reward);
		}

		void ResetModel() const
		{
			for (ndInt32 i = 0; i < m_basePose.GetCount(); i++)
			{
				m_basePose[i].SetPose();
			}
			m_legJoint->SetTargetAngle(0.0f);
			m_wheelJoint->SetTargetAngle(0.0f);
		}

		bool IsTerminal() const
		{
			#define D_REWARD_MIN_ANGLE	(ndFloat32 (20.0f) * ndDegreeToRad)
			const ndMatrix& matrix = GetRoot()->m_body->GetMatrix();
			ndFloat32 sinAngle = ndClamp(matrix.m_up.m_x, ndFloat32 (-0.9f), ndFloat32(0.9f));
			bool fail = ndAbs(ndAsin(sinAngle)) > (D_REWARD_MIN_ANGLE * ndFloat32(2.0f));
			return fail;
		}

		bool IsOutOfBounds() const
		{
			ndBodyKinematic* const body = GetRoot()->m_body->GetAsBodyKinematic();
			return ndAbs(body->GetMatrix().m_posit.m_x) > ndFloat32(20.0f);
		}

		void TelePort() const
		{
			ndBodyKinematic* const body = GetRoot()->m_body->GetAsBodyKinematic();

			ndVector veloc(body->GetVelocity());
			ndVector posit(body->GetMatrix().m_posit);
			posit.m_y = 0.0f;
			posit.m_z = 0.0f;
			posit.m_w = 0.0f;
			for (ndInt32 i = 0; i < m_bodies.GetCount(); ++i)
			{
				ndBodyKinematic* const modelBody = m_bodies[i];
				modelBody->SetVelocity(modelBody->GetVelocity() - veloc);
				ndMatrix matrix(modelBody->GetMatrix());
				matrix.m_posit -= posit;
				modelBody->SetMatrix(matrix);
			}
		}

		bool HasSupportContact() const
		{
			ndBodyKinematic* const body = m_wheelJoint->GetBody0();
			ndBodyKinematic::ndContactMap::Iterator it(body->GetContactMap());
			for (it.Begin(); it; it++)
			{
				ndContact* const contact = it.GetNode()->GetInfo();
				if (contact->IsActive())
				{
					return true;
				}
			}
			return false;
		}

		void CheckTrainingCompleted()
		{
			#ifdef ND_TRAIN_MODEL
			if (m_agent->IsTrainer())
			{
				ndControllerAgent_trainer* const agent = (ndControllerAgent_trainer*)*m_agent;
				if (agent->m_modelIsTrained)
				{
					char fileName[1024];
					ndGetWorkingFileName("unicycle.dnn", fileName);
					ndSharedPtr<ndBrain> actor(ndBrainLoad::Load(fileName));
					m_agent = ndSharedPtr<ndBrainAgent>(new ndModelUnicycle::ndControllerAgent(actor));
					((ndModelUnicycle::ndControllerAgent*)*m_agent)->SetModel(this);
					ResetModel();
					((ndPhysicsWorld*)m_world)->NormalUpdates();
				}
			}
			#endif
		}

		void Update(ndWorld* const world, ndFloat32 timestep)
		{
			ndModelArticulation::Update(world, timestep);
			m_timestep = timestep;
			m_agent->Step();
		}

		void PostUpdate(ndWorld* const world, ndFloat32 timestep)
		{
			ndModelArticulation::PostUpdate(world, timestep);
			m_agent->OptimizeStep();
			CheckTrainingCompleted();
		}

		ndSharedPtr<ndBrainAgent> m_agent;
		ndFixSizeArray<ndBasePose, 8> m_basePose;
		ndFixSizeArray<ndBodyDynamic*, 8> m_bodies;
		ndJointHinge* m_legJoint;
		ndJointHinge* m_wheelJoint;
		ndFloat32 m_timestep;
	};

	void BuildModel(ndModelUnicycle* const model, ndDemoEntityManager* const scene, const ndMatrix& location)
	{
		ndFloat32 mass = 20.0f;
		ndFloat32 limbMass = 1.0f;
		ndFloat32 wheelMass = 1.0f;

		ndFloat32 xSize = 0.25f;
		ndFloat32 ySize = 0.40f;
		ndFloat32 zSize = 0.30f;
		ndPhysicsWorld* const world = scene->GetWorld();
		
		// add hip body
		ndSharedPtr<ndBody> hipBody(world->GetBody(AddBox(scene, location, mass, xSize, ySize, zSize, "smilli.tga")));
		ndModelArticulation::ndNode* const modelRoot = model->AddRootBody(hipBody);

		ndMatrix matrix(hipBody->GetMatrix());
		matrix.m_posit.m_y += 0.5f;
		hipBody->SetMatrix(matrix);

		ndMatrix limbLocation(matrix);
		limbLocation.m_posit.m_z += zSize * 0.0f;
		limbLocation.m_posit.m_y -= ySize * 0.5f;

		// make single leg
		ndFloat32 limbLength = 0.3f;
		ndFloat32 limbRadio = 0.025f;

		ndSharedPtr<ndBody> legBody(world->GetBody(AddCapsule(scene, ndGetIdentityMatrix(), limbMass, limbRadio, limbRadio, limbLength, "smilli.tga")));
		ndMatrix legLocation(ndRollMatrix(-90.0f * ndDegreeToRad) * limbLocation);
		legLocation.m_posit.m_y -= limbLength * 0.5f;
		legBody->SetMatrix(legLocation);
		ndMatrix legPivot(ndYawMatrix(90.0f * ndDegreeToRad) * legLocation);
		legPivot.m_posit.m_y += limbLength * 0.5f;
		ndSharedPtr<ndJointBilateralConstraint> legJoint(new ndJointHinge(legPivot, legBody->GetAsBodyKinematic(), modelRoot->m_body->GetAsBodyKinematic()));
		ndJointHinge* const hinge = (ndJointHinge*)*legJoint;
		hinge->SetAsSpringDamper(0.02f, 1500, 40.0f);
		model->m_legJoint = hinge;

		// make wheel
		ndFloat32 wheelRadio = 4.0f * limbRadio;
		ndSharedPtr<ndBody> wheelBody(world->GetBody(AddSphere(scene, ndGetIdentityMatrix(), wheelMass, wheelRadio, "smilli.tga")));
		ndMatrix wheelMatrix(legPivot);
		wheelMatrix.m_posit.m_y -= limbLength;
		wheelBody->SetMatrix(wheelMatrix);
		ndSharedPtr<ndJointBilateralConstraint> wheelJoint(new ndJointHinge(wheelMatrix, wheelBody->GetAsBodyKinematic(), legBody->GetAsBodyKinematic()));
		ndJointHinge* const wheelMotor = (ndJointHinge*)*wheelJoint;
		wheelMotor->SetAsSpringDamper(0.02f, 0.0f, 0.2f);
		model->m_wheelJoint = wheelMotor;

		// tele port the model so that is on the floor
		ndMatrix probeMatrix(wheelMatrix);
		probeMatrix.m_posit.m_x += 1.0f;
		ndMatrix floor(FindFloor(*world, probeMatrix, wheelBody->GetAsBodyKinematic()->GetCollisionShape(), 20.0f));
		ndFloat32 dist = wheelMatrix.m_posit.m_y - floor.m_posit.m_y;

		ndMatrix rootMatrix(modelRoot->m_body->GetMatrix());

		rootMatrix.m_posit.m_y -= dist;
		wheelMatrix.m_posit.m_y -= dist;
		legLocation.m_posit.m_y -= dist;

		legBody->SetMatrix(legLocation);
		wheelBody->SetMatrix(wheelMatrix);
		modelRoot->m_body->SetMatrix(rootMatrix);

		legBody->GetNotifyCallback()->OnTransform(0, legLocation);
		wheelBody->GetNotifyCallback()->OnTransform(0, wheelMatrix);
		modelRoot->m_body->GetNotifyCallback()->OnTransform(0, rootMatrix);

		world->AddJoint(legJoint);
		world->AddJoint(wheelJoint);

		// add model limbs
		ndModelArticulation::ndNode* const legLimb = model->AddLimb(modelRoot, legBody, legJoint);
		model->AddLimb(legLimb, wheelBody, wheelJoint);

		model->m_bodies.SetCount(0);
		model->m_basePose.SetCount(0);
		for (ndModelArticulation::ndNode* node = model->GetRoot()->GetFirstIterator(); node; node = node->GetNextIterator())
		{
			ndBodyDynamic* const body = node->m_body->GetAsBodyDynamic();
			model->m_bodies.PushBack(body);
			model->m_basePose.PushBack(body);
		}
	}

	ndModelArticulation* CreateModel(ndDemoEntityManager* const scene, const ndMatrix& location)
	{
		// build neural net controller
		#ifdef ND_TRAIN_MODEL
			ndInt32 layerSize = 64;
			//ndBrainActivationType activation = m_tanh;
			//ndBrainActivationType activation = m_relu;

			ndSharedPtr<ndBrain> actor(new ndBrain());
			ndAssert(0);
			//ndBrainLayerLinearActivated* const layer0 = new ndBrainLayerLinearActivated(m_stateSize, layerSize, activation);
			//ndBrainLayerLinearActivated* const layer1 = new ndBrainLayerLinearActivated(layer0->GetOutputSize(), layerSize, activation);
			//ndBrainLayerLinearActivated* const layer2 = new ndBrainLayerLinearActivated(layer1->GetOutputSize(), layerSize, activation);
			//ndBrainLayerLinearActivated* const ouputLayer = new ndBrainLayerLinearActivated(layer2->GetOutputSize(), m_actionsSize, m_tanh);
			//
			//actor->AddLayer(layer0);
			//actor->AddLayer(layer1);
			//actor->AddLayer(layer2);
			//actor->AddLayer(ouputLayer);

			// the critic is more complex since is deal with more complex inputs
			ndSharedPtr<ndBrain> critic(new ndBrain());
			ndAssert(0);
			//ndBrainLayerLinearActivated* const criticLayer0 = new ndBrainLayerLinearActivated(m_stateSize + m_actionsSize, layerSize * 2, activation);
			//ndBrainLayerLinearActivated* const criticLayer1 = new ndBrainLayerLinearActivated(criticLayer0->GetOutputSize(), layerSize * 2, activation);
			//ndBrainLayerLinearActivated* const criticLayer2 = new ndBrainLayerLinearActivated(criticLayer1->GetOutputSize(), layerSize * 2, activation);
			//ndBrainLayerLinearActivated* const criticOuputLayer = new ndBrainLayerLinearActivated(criticLayer2->GetOutputSize(), 1, m_relu);
			//
			//critic->AddLayer(criticLayer0);
			//critic->AddLayer(criticLayer1);
			//critic->AddLayer(criticLayer2);
			//critic->AddLayer(criticOuputLayer);

			// add a reinforcement learning controller 
			#ifdef USE_TD3
			ndBrainAgentTD3_Trainer<m_stateSize, m_actionsSize>::HyperParameters hyperParameters;
			#else
			ndBrainAgentDDPG_Trainer<m_stateSize, m_actionsSize>::HyperParameters hyperParameters;
			#endif
			//hyperParameters.m_discountFactor = ndReal (0.995f);
			//hyperParameters.m_threadsCount = 1;

			ndSharedPtr<ndBrainAgent> agent(new ndModelUnicycle::ndControllerAgent_trainer(hyperParameters, actor, critic));
			agent->SetName("unicycle.dnn");
			scene->SetAcceleratedUpdate();
		#else
			char fileName[1024];
			ndGetWorkingFileName("unicycle.dnn", fileName);
			ndSharedPtr<ndBrain> actor(ndBrainLoad::Load(fileName));
			ndSharedPtr<ndBrainAgent> agent(new  ndModelUnicycle::ndControllerAgent(actor));
		#endif

		ndModelUnicycle* const model = new ndModelUnicycle(agent);
		BuildModel(model, scene, location);
		#ifdef ND_TRAIN_MODEL
			((ndModelUnicycle::ndControllerAgent_trainer*)*agent)->SetModel(model);
		#else
			((ndModelUnicycle::ndControllerAgent*)*agent)->SetModel(model);
		#endif
		return model;
	}
}

using namespace ndController_1;
void ndBalanceController(ndDemoEntityManager* const scene)
{
	// build a floor
	//BuildFloorBox(scene, ndGetIdentityMatrix());
	BuildFlatPlane(scene, true);
	
	ndWorld* const world = scene->GetWorld();
	ndMatrix matrix(ndYawMatrix(-0.0f * ndDegreeToRad));

	ndSharedPtr<ndModel> model(CreateModel(scene, matrix));
	scene->GetWorld()->AddModel(model);

	ndModelArticulation* const articulation = (ndModelArticulation*)model->GetAsModelArticulation();
	ndBodyKinematic* const rootBody = articulation->GetRoot()->m_body->GetAsBodyKinematic();
	ndSharedPtr<ndJointBilateralConstraint> fixJoint(new ndJointPlane(rootBody->GetMatrix().m_posit, ndVector(0.0f, 0.0f, 1.0f, 0.0f), rootBody, world->GetSentinelBody()));
	world->AddJoint(fixJoint);
	
	matrix.m_posit.m_x -= 0.0f;
	matrix.m_posit.m_y += 0.5f;
	matrix.m_posit.m_z += 3.0f;
	ndQuaternion rotation(ndVector(0.0f, 1.0f, 0.0f, 0.0f), 90.0f * ndDegreeToRad);
	scene->SetCameraMatrix(rotation, matrix.m_posit);
}
