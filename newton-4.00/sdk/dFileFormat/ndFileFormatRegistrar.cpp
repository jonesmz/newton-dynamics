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
#include "ndFileFormatRegistrar.h"
#include "ndFileFormatBody.h"
#include "ndFileFormatShape.h"
#include "ndFileFormatNotify.h"
#include "ndFileFormatShapeConvex.h"
#include "ndFileFormatDynamicBody.h"
#include "ndFileFormatKinematicBody.h"
#include "ndFileFormatShapeInstance.h"
#include "ndFileFormatShapeCompound.h"
#include "ndFileFormatShapeConvexBox.h"
#include "ndFileFormatShapeConvexCone.h"
#include "ndFileFormatShapeConvexHull.h"
#include "ndFileFormatShapeStaticMesh.h"
#include "ndFileFormatBodyTriggerVolume.h"
#include "ndFileFormatKinematicBodyBase.h"
#include "ndFileFormatShapeConvexSphere.h"
#include "ndFileFormatShapeConvexCapsule.h"
#include "ndFileFormatShapeConvexCylinder.h"
#include "ndFileFormatShapeStaticMesh_bvh.h"
#include "ndFileFormatShapeStaticHeightfield.h"
#include "ndFileFormatShapeConvexChamferCylinder.h"

ndFixSizeArray<ndFileFormatRegistrar*, 256> ndFileFormatRegistrar::m_registry;

ndFileFormatRegistrar::ndFileFormatRegistrar(const char* const className)
	:ndClassAlloc()
	,m_hash(dCRC64(className))
{
	ndAssert(!GetHandler(className));

	m_registry.PushBack(this);
	for (ndInt32 i = m_registry.GetCount() - 2; i >= 0; --i)
	{
		ndFileFormatRegistrar* const entry = m_registry[i];
		if (entry->m_hash > m_hash)
		{
			m_registry[i] = this;
			m_registry[i + 1] = entry;
		}
		else
		{
			break;
		}
	}
}

ndFileFormatRegistrar::~ndFileFormatRegistrar()
{
}

void ndFileFormatRegistrar::Init()
{
	static ndFileFormatBody body;
	static ndFileFormatShape shape;
	static ndFileFormatNotify bodyNotiy;
	static ndFileFormatDynamicBody dynamicBody;
	static ndFileFormatShapeInstance collision;
	static ndFileFormatKinematicBody kinematicBody;
	static ndFileFormatBodyTriggerVolume triggerVolume;
	static ndFileFormatKinematicBodyBase kinematicBodyBase;

	static ndFileFormatShapeConvex shapeConvex;
	static ndFileFormatShapeConvexBox shapeBox;
	static ndFileFormatShapeConvexCone shapeCone;
	static ndFileFormatShapeCompound shapeCompound;
	static ndFileFormatShapeConvexSphere shapeSphere;
	static ndFileFormatShapeConvexCapsule shapeCapsule;
	static ndFileFormatShapeConvexHull shapeConvexHull;
	static ndFileFormatShapeStaticMesh shapeStaticMesh;
	static ndFileFormatShapeConvexCylinder shapeCylinder;
	static ndFileFormatShapeStaticMesh_bvh shapeStaticBvh;
	static ndFileFormatShapeStaticHeightfield shapeHeightfield;
	static ndFileFormatShapeConvexChamferCylinder shapeChamferCylinder;
}

ndFileFormatRegistrar* ndFileFormatRegistrar::GetHandler(const char* const className)
{
	ndUnsigned64 hash = dCRC64(className);
	ndInt32 i0 = 0;
	ndInt32 i1 = m_registry.GetCount() - 1;
	while ((i1 - i0 > 4))
	{
		ndInt32 mid = (i1 + i0) / 2;
		if (m_registry[mid]->m_hash <= hash)
		{
			i0 = mid;
		}
		else
		{
			i1 = mid;
		}
	}
	
	for (ndInt32 i = i0; i <= i1; ++i)
	{
		if (m_registry[i]->m_hash == hash)
		{
			return m_registry[i];
		}
	}

	return nullptr;
}

void ndFileFormatRegistrar::SaveBody(ndFileFormat* const, nd::TiXmlElement* const, const ndBody* const)
{
	ndAssert(0);
}

void ndFileFormatRegistrar::SaveNotify(ndFileFormat* const, nd::TiXmlElement* const, const ndBodyNotify* const)
{
	ndAssert(0);
}

ndInt32 ndFileFormatRegistrar::SaveShape(ndFileFormat* const, nd::TiXmlElement* const, const ndShape* const)
{
	ndAssert(0);
	return 0;
}

void ndFileFormatRegistrar::SaveCollision(ndFileFormat* const, nd::TiXmlElement* const, const ndShapeInstance* const)
{
	ndAssert(0);
}