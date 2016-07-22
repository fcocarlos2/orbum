#pragma once

#include "Globals.h"

#include "VMExceptionHandlerComponent.h"

/*
TODO: Fill in documentation.
*/

class VMMain;
class PS2Exception_t;

class ExceptionHandler : public VMExceptionHandlerComponent
{
public:
	explicit ExceptionHandler(const VMMain *const vmMain);

	void handleException(const PS2Exception_t& PS2Exception);
};
