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
			CollisionThread = 1 << 1,

			CollisionAll = 0xffffffff
		};
		
		enum MaterialType
		{
			MaterialDefault,
			MaterialGround,
			MaterialMoving,
			MaterialWater,
			MaterialPlayer
		};
	}
}

#endif /* defined(__PF_TYPES_H_) */
