/*
 *
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #####################################################
//#######################################################
//###                                                 ###
//##    WARNING: This file is generated. DO NOT EDIT   ##
//##             All changes will be lost!             ##
//###                                                 ###
//#######################################################
// #####################################################
#include "joynr/tests/MultipleVersionsInterface1Provider.h"
#include "joynr/InterfaceRegistrar.h"

#include "joynr/tests/MultipleVersionsInterface1RequestInterpreter.h"
#include "joynr/tests/InterfaceVersionedStruct1.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct1.h"
#include "joynr/tests/AnonymousVersionedStruct1.h"

namespace joynr { namespace tests { 
MultipleVersionsInterface1Provider::MultipleVersionsInterface1Provider()
{
	// Register a request interpreter to interpret requests to this interface
	joynr::InterfaceRegistrar::instance().registerRequestInterpreter<MultipleVersionsInterface1RequestInterpreter>(INTERFACE_NAME() + std::to_string(MAJOR_VERSION));
}

MultipleVersionsInterface1Provider::~MultipleVersionsInterface1Provider()
{
	// Unregister the request interpreter
	joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(
			INTERFACE_NAME() + std::to_string(MAJOR_VERSION)
	);
}

const std::string& MultipleVersionsInterface1Provider::INTERFACE_NAME()
{
	static const std::string INTERFACE_NAME("tests/MultipleVersionsInterface");
	return INTERFACE_NAME;
}

const std::uint32_t MultipleVersionsInterface1Provider::MAJOR_VERSION = 1;
const std::uint32_t MultipleVersionsInterface1Provider::MINOR_VERSION = 0;


} // namespace tests
} // namespace joynr