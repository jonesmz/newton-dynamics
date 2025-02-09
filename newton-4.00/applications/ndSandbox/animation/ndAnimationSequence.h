/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __D_ANIMIMATION_SEQUENCE_h__
#define __D_ANIMIMATION_SEQUENCE_h__

#include "ndSandboxStdafx.h"
#include "ndAnimationSequenceBase.h"
#include "ndAnimationKeyframesTrack.h"

class ndAnimationSequence: public ndAnimationSequenceBase
{
	public:
	ndAnimationSequence();
	virtual ~ndAnimationSequence();

	ndAnimationKeyFramesTrack* AddTrack();
	ndList<ndAnimationKeyFramesTrack>& GetTracks();
	virtual void CalculatePose(ndAnimationPose& output, ndFloat32 param) const;

	private:
	ndList<ndAnimationKeyFramesTrack> m_tracks;
};

#endif