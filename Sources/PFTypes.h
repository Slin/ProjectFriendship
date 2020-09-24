//
//  PFTypes.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_TYPES_H_
#define __PF_TYPES_H_

#include "Rayne.h"

namespace PF
{
	namespace Types
	{
		enum CollisionType
		{
			CollisionLevel = 1 << 0,
			CollisionPlants = 1 << 1,
			CollisionThread = 1 << 2,
			CollisionAirbubble = 1 << 3,
			CollisionPlayer = 1 << 4,

			CollisionAll = 0xffffffff,
			CollisionPlayerMask = CollisionLevel|CollisionPlants,
			CollisionGravityMask = CollisionLevel
		};
		
		enum MaterialType
		{
			MaterialDefault,
			MaterialGround,
			MaterialMoving,
			MaterialWater,
			MaterialPlayer,
			MaterialAirbubble
		};
	}
}

#endif /* defined(__PF_TYPES_H_) */
