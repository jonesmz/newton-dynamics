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

#include "ndFileFormatStdafx.h"
#include "ndFileFormat.h"
#include "ndFileFormatRegistrar.h"

ndFileFormat::ndFileFormat()
	:ndClassAlloc()
	,m_bodies()
	,m_joints()
	,m_bodiesIds()
	,m_jointsIds()
	,m_uniqueShapesIds()
{
	xmlReserClassId();
	ndFileFormatRegistrar::Init();
}

ndFileFormat::~ndFileFormat()
{
}

void ndFileFormat::CollectScene(const ndWorld* const world)
{
	m_world = world;
	m_joints.SetCount(0);
	m_bodies.SetCount(0);

	for (ndBodyListView::ndNode* node = world->GetBodyList().GetFirst(); node; node = node->GetNext())
	{
		ndBody* const body = *node->GetInfo();
		m_bodies.PushBack(body);
	}

	for (ndJointList::ndNode* node = world->GetJointList().GetFirst(); node; node = node->GetNext())
	{
		ndJointBilateralConstraint* const joint = *node->GetInfo();
		m_joints.PushBack(joint);
	}
}

void ndFileFormat::SaveCollisionShapes(nd::TiXmlElement* const rootNode)
{
	// save bodies without compound shapes.
	if (m_bodies.GetCount())
	{
		nd::TiXmlElement* const shapeNode = new nd::TiXmlElement("ndShape");
		rootNode->LinkEndChild(shapeNode);
		for (ndInt32 i = 0; i < m_bodies.GetCount(); ++i)
		{
			ndBodyKinematic* const body = m_bodies[i]->GetAsBodyKinematic();
			ndShape* const shape = body->GetCollisionShape().GetShape();
			if (!shape->GetAsShapeCompound())
			{
				ndUnsigned64 hash = shape->GetHash();
				ndTree<ndInt32, ndUnsigned64>::ndNode* const node = m_uniqueShapesIds.Insert(hash);
				if (node)
				{
					ndFileFormatRegistrar* const handler = ndFileFormatRegistrar::GetHandler(shape->ClassName());
					if (!handler)
					{
						ndTrace(("failed to save shape type: %s\n", shape->ClassName()));
						ndAssert(0);
					}

					ndInt32 id = handler->SaveShape(this, shapeNode, shape);
					node->GetInfo() = id;
				}
			}
		}

		// save bodies with compound shapes.
		for (ndInt32 i = 0; i < m_bodies.GetCount(); ++i)
		{
			ndBodyKinematic* const body = m_bodies[i]->GetAsBodyKinematic();
			ndShape* const shape = body->GetCollisionShape().GetShape();
			if (shape->GetAsShapeCompound())
			{
				ndUnsigned64 hash = shape->GetHash();
				ndTree<ndInt32, ndUnsigned64>::ndNode* const node = m_uniqueShapesIds.Insert(hash);
				if (node)
				{
					ndFileFormatRegistrar* const handler = ndFileFormatRegistrar::GetHandler(shape->ClassName());
					ndInt32 id = handler->SaveShape(this, shapeNode, shape);
					node->GetInfo() = id;
				}
			}
		}
	}
}

void ndFileFormat::SaveBodies(nd::TiXmlElement* const rootNode)
{
	if (m_bodies.GetCount())
	{
		nd::TiXmlElement* const bodiesNode = new nd::TiXmlElement("ndBodies");
		rootNode->LinkEndChild(bodiesNode);

		for (ndInt32 i = 0; i < m_bodies.GetCount(); ++i)
		{
			ndBody* const body = m_bodies[i];
			ndFileFormatRegistrar* const handler = ndFileFormatRegistrar::GetHandler(body->ClassName());
			ndAssert(handler);
			if (!handler)
			{
				ndTrace(("failed to save body type: %s\n", body->ClassName()));
				ndAssert(0);
			}
			if (handler)
			{
				handler->SaveBody(this, bodiesNode, body);
			}
		}
	}
}

void ndFileFormat::SaveJoints(nd::TiXmlElement* const rootNode)
{
	if (m_joints.GetCount())
	{
		nd::TiXmlElement* const jointsNode = new nd::TiXmlElement("ndJoints");
		rootNode->LinkEndChild(jointsNode);

		for (ndInt32 i = 0; i < m_joints.GetCount(); ++i)
		{
			ndJointBilateralConstraint* const joint = m_joints[i];
			ndFileFormatRegistrar* const handler = ndFileFormatRegistrar::GetHandler(joint->ClassName());
			if (!handler)
			{
				ndTrace(("failed to save joint type: %s\n", joint->ClassName()));
				ndAssert(0);
			}
			if (handler)
			{
				handler->SaveJoint(this, jointsNode, joint);
			}
		}
	}
}

void ndFileFormat::SaveWorld(nd::TiXmlElement* const rootNode)
{
	ndFileFormatRegistrar* const handler = ndFileFormatRegistrar::GetHandler(m_world->ClassName());
	ndAssert(handler);
	if (handler)
	{
		nd::TiXmlElement* const worldNode = new nd::TiXmlElement("ndWorld");
		rootNode->LinkEndChild(worldNode);
		handler->SaveWorld(this, worldNode, m_world);
	}
}

void ndFileFormat::SaveBodies(const ndWorld* const world, const char* const path)
{
	// save the path for use with generated assets.
	m_fileName = path;
	
	nd::TiXmlDocument asciifile;
	nd::TiXmlDeclaration* const decl = new nd::TiXmlDeclaration("1.0", "", "");
	asciifile.LinkEndChild(decl);
	
	nd::TiXmlElement* const rootNode = new nd::TiXmlElement("ndFile");
	asciifile.LinkEndChild(rootNode);

	m_jointsIds.RemoveAll();
	m_bodiesIds.RemoveAll();
	m_uniqueShapesIds.RemoveAll();

	CollectScene(world);
	SaveCollisionShapes(rootNode);
	SaveBodies(rootNode);
	
	char* const oldloc = setlocale(LC_ALL, 0);
	setlocale(LC_ALL, "C");
	asciifile.SaveFile(path);
	setlocale(LC_ALL, oldloc);
}

void ndFileFormat::SaveWorld(const ndWorld* const world, const char* const path)
{
	m_fileName = path;

	nd::TiXmlDocument asciifile;
	nd::TiXmlDeclaration* const decl = new nd::TiXmlDeclaration("1.0", "", "");
	asciifile.LinkEndChild(decl);

	nd::TiXmlElement* const rootNode = new nd::TiXmlElement("ndFile");
	asciifile.LinkEndChild(rootNode);

	m_jointsIds.RemoveAll();
	m_bodiesIds.RemoveAll();
	m_uniqueShapesIds.RemoveAll();

	CollectScene(world);
	SaveWorld(rootNode);
	SaveCollisionShapes(rootNode);
	SaveBodies(rootNode);
	SaveJoints(rootNode);

	char* const oldloc = setlocale(LC_ALL, 0);
	setlocale(LC_ALL, "C");
	asciifile.SaveFile(path);
	setlocale(LC_ALL, oldloc);
}