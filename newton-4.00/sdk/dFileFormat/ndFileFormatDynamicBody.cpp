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
#include "ndFileFormatDynamicBody.h"

ndFileFormatDynamicBody::ndFileFormatDynamicBody()
	:ndFileFormatKinematicBody(ndBodyDynamic::StaticClassName())
{
}

ndFileFormatDynamicBody::ndFileFormatDynamicBody(const char* const className)
	:ndFileFormatKinematicBody(className)
{
}

void ndFileFormatDynamicBody::SaveBody(ndFileFormatSave* const scene, nd::TiXmlElement* const parentNode, const ndBody* const body)
{
	nd::TiXmlElement* const classNode = xmlCreateClassNode(parentNode, "ndBodyClass", ndBodyDynamic::StaticClassName());
	ndFileFormatKinematicBody::SaveBody(scene, classNode, body);

	const ndBodyDynamic* const dynamic = ((ndBodyDynamic*)body)->GetAsBodyDynamic();
	xmlSaveParam(classNode, "linearDampCoef", dynamic->m_dampCoef.m_w);
	xmlSaveParam(classNode, "angularDampCoef", dynamic->m_dampCoef);
}


ndBody* ndFileFormatDynamicBody::LoadBody(const nd::TiXmlElement* const node, const ndTree<ndShape*, ndInt32>& shapeMap)
{
	ndBodyDynamic* const body = new ndBodyDynamic();
	LoadBody(node, shapeMap, body);

	return body;
}

void ndFileFormatDynamicBody::LoadBody(const nd::TiXmlElement* const node, const ndTree<ndShape*, ndInt32>& shapeMap, ndBody* const body)
{
	ndFileFormatKinematicBody::LoadBody((nd::TiXmlElement*)node->FirstChild("ndBodyClass"), shapeMap, body);

	ndFloat32 linearDamp = xmlGetFloat(node, "linearDampCoef");
	ndVector angularDamp(xmlGetVector3(node, "angularDampCoef"));

	ndBodyDynamic* const dynBody = (ndBodyDynamic*)body;
	dynBody->SetLinearDamping(linearDamp);
	dynBody->SetAngularDamping(angularDamp);
}