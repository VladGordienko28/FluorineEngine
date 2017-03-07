/*=============================================================================
    FrEngine.h: Engine general include file.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/
#ifndef _FLU_ENGINE_
#define _FLU_ENGINE_

// C++ includes.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdarg.h>
#include <stdio.h>
   
// Partial classes tree.
class String;
class CClass;
class CSerializer;
class FObject;
	class FEntity;
	class FComponent;
		class FBaseComponent;
			class FCameraComponent;
			class FZoneComponent;
			class FPhysicComponent;
		class FExtraComponent;
			class FLogicComponent;
	class FResource;
		class FFont;
		class FBlockResource;
			class FBitmap;
		class FLevel;
		class FScript;
		class FAnimation;
		class FProjectInfo;
class CCanvas;
class CBlockManager;
class CInstanceBuffer;
class CFrame;
class CEntityThread;
class CCollisionHash;
class CNavigator;
class CPhysics;
enum EPathType;
enum EEventName;
template<class T> class TArray;
template<class K, class V> class TMap;


// Engine includes.
#include "FrBuild.h"
#include "FrBase.h"
#include "FrString.h"
#include "FrLog.h"
#include "FrRand.h"
#include "FrStaMem.h"
#include "FrSerial.h"
#include "FrMath.h"
#include "FrColor.h"
#include "FrExpImp.h"
#include "FrArray.h"
#include "FrMap.h"
#include "FrEncode.h"
#include "FrClass.h"
#include "FrObject.h"
#include "FrFile.h"
#include "FrIni.h"
#include "FrCom.h"
#include "FrEntity.h"
#include "FrBlock.h"
#include "FrRes.h"
#include "FrAudio.h"
#include "FrOpCode.h"
#include "FrScript.h"
#include "FrCode.h"
#include "FrBitmap.h"
#include "FrAnim.h"
#include "FrFont.h"
#include "FrRender.h"
#include "FrComTyp.h"
#include "FrGFX.h"
#include "FrInput.h"
#include "FrLevel.h"
#include "FrCollHash.h"
#include "FrProject.h"
#include "FrApp.h"
#include "FrDemoEff.h"
#include "FrPhysEng.h"
#include "FrPath.h"


#endif
/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/