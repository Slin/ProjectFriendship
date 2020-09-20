//
//  PFApplication.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_APPLICATION_H_
#define __PF_APPLICATION_H_

#include <Rayne.h>
#include "RNVRApplication.h"

namespace PF
{
	class Application : public RN::VRApplication
	{
	public:
		Application();
		~Application();
		
		void WillFinishLaunching(RN::Kernel *kernel) override;
		void DidFinishLaunching(RN::Kernel *kernel) override;
	};
}


#endif /* __PF_APPLICATION_H_ */
