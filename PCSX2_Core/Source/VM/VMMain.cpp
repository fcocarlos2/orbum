#include "stdafx.h"

#include <memory>

#include "Common/Global/Globals.h"
#include "VM/VMMain.h"
#include "VM/ExecutionCore/Interpreter/Interpreter.h"
#include "PS2Resources/PS2Resources_t.h"
#include "PS2Resources/EE/EE_t.h"
#include "PS2Resources/EE/Types/BootROM_t.h"

VMMain::VMMain(const VMOptions_t & vmOptions) : 
	mStatus(VMStatus::CREATED),
	mVMOptions(vmOptions)
{
	// Initialise everything.
	Reset();
}

void VMMain::Reset()
{
	// Set the log delegate function.
	logDelegate = mVMOptions.LOG_DELEGATE_FUNCPTR;

	// Initalise resources.
	initaliseResources();

	// Initiaise ROMs.
	initaliseROMs();

	// Initalise the execution core class.
	initaliseExecutionCore();

	// Initalise the execution core & components.
	mExecutionCore->initalise();
}

void VMMain::Run()
{
	// Set to running.
	mStatus = VMStatus::RUNNING;

	// Run the VM.
	while (mStatus == VMStatus::RUNNING)
	{
		mExecutionCore->executionStep();
	}
}

void VMMain::Stop()
{
	// TODO: Implement.
}

VMMain::~VMMain()
{
}

const VMMain::VMStatus& VMMain::getStatus() const
{
	return mStatus;
}

void VMMain::setStatus(const VMStatus& status)
{
	mStatus = status;
}

const std::shared_ptr<PS2Resources_t> & VMMain::getResources() const
{
	return mPS2Resources;
}

void VMMain::initaliseResources()
{
	mPS2Resources = std::make_shared<PS2Resources_t>();
}

void VMMain::initaliseExecutionCore()
{
	switch (mVMOptions.OPTION_EXECUTION_CORE)
	{
	case ExecutionCore_t::Interpreter:
	{
		mExecutionCore = std::make_unique<Interpreter>(this);
		break;
	}
	case ExecutionCore_t::Recompiler:
	{
		// TODO: Implement.
		break;
	}
	}
}

void VMMain::initaliseROMs() const
{
	// Initalise BootROM.
	getResources()->EE->BootROM->loadBIOS(mVMOptions.BOOT_ROM_PATH);

	// Initalise ROM1.
	// Initalise EROM.
	// Initalise ROM2.
}
