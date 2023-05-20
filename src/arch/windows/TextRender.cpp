// msp.cpp : main source file for msp.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlscrl.h>
#include <memory>

#include "svg.h"
#include "resource.h"
#include "mspwin.h"

////////////////////////////////////////
// COM help.

// Releases a COM object and nullifies pointer.
template <typename InterfaceType>
inline void SafeRelease(InterfaceType** currentObject)
{
	if (*currentObject != NULL)
	{
		(*currentObject)->Release();
		*currentObject = NULL;
	}
}

// Acquires an additional reference, if non-null.
template <typename InterfaceType>
inline InterfaceType* SafeAcquire(InterfaceType* newObject)
{
	if (newObject != NULL)
		newObject->AddRef();

	return newObject;
}

// Sets a new COM object, releasing the old one.
template <typename InterfaceType>
inline void SafeSet(InterfaceType** currentObject, InterfaceType* newObject)
{
	SafeAcquire(newObject);
	SafeRelease(currentObject);
	*currentObject = newObject;
}

// Releases a COM object and nullifies pointer.
template <typename InterfaceType>
inline InterfaceType* SafeDetach(InterfaceType** currentObject)
{
	InterfaceType* oldObject = *currentObject;
	*currentObject = NULL;
	return oldObject;
}

// Sets a new COM object, acquiring the reference.
template <typename InterfaceType>
inline void SafeAttach(InterfaceType** currentObject, InterfaceType* newObject)
{
	SafeRelease(currentObject);
	*currentObject = newObject;
}



