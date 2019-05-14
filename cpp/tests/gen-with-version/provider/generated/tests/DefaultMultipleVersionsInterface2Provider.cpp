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
#include "joynr/tests/DefaultMultipleVersionsInterface2Provider.h"

#include <chrono>
#include <cstdint>
#include <tuple>


namespace joynr { namespace tests { 

DefaultMultipleVersionsInterface2Provider::DefaultMultipleVersionsInterface2Provider() :
		MultipleVersionsInterface2AbstractProvider()
		,
		uInt8Attribute1(),
		uInt8Attribute2()
{
}

DefaultMultipleVersionsInterface2Provider::~DefaultMultipleVersionsInterface2Provider() = default;

// attributes
void DefaultMultipleVersionsInterface2Provider::getUInt8Attribute1(
		std::function<void(
				const std::uint8_t&
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	onSuccess(uInt8Attribute1);
}

void DefaultMultipleVersionsInterface2Provider::setUInt8Attribute1(
		const std::uint8_t& uInt8Attribute1,
		std::function<void()> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	this->uInt8Attribute1 = uInt8Attribute1;
	uInt8Attribute1Changed(uInt8Attribute1);
	onSuccess();
}

void DefaultMultipleVersionsInterface2Provider::getUInt8Attribute2(
		std::function<void(
				const std::uint8_t&
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	onSuccess(uInt8Attribute2);
}

void DefaultMultipleVersionsInterface2Provider::setUInt8Attribute2(
		const std::uint8_t& uInt8Attribute2,
		std::function<void()> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	this->uInt8Attribute2 = uInt8Attribute2;
	uInt8Attribute2Changed(uInt8Attribute2);
	onSuccess();
}

// methods
void DefaultMultipleVersionsInterface2Provider::getTrue(
		std::function<void(
				const bool& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	bool result = false;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterface2Provider::getTrue called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}

void DefaultMultipleVersionsInterface2Provider::getVersionedStruct(
		const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& input,
		std::function<void(
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	std::ignore = input;
	joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2 result;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterface2Provider::getVersionedStruct called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}

void DefaultMultipleVersionsInterface2Provider::getAnonymousVersionedStruct(
		const joynr::tests::AnonymousVersionedStruct2& input,
		std::function<void(
				const joynr::tests::AnonymousVersionedStruct2& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	std::ignore = input;
	joynr::tests::AnonymousVersionedStruct2 result;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterface2Provider::getAnonymousVersionedStruct called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}

void DefaultMultipleVersionsInterface2Provider::getInterfaceVersionedStruct(
		const joynr::tests::InterfaceVersionedStruct2& input,
		std::function<void(
				const joynr::tests::InterfaceVersionedStruct2& result
		)> onSuccess,
		std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
	std::ignore = onError;
	std::ignore = input;
	joynr::tests::InterfaceVersionedStruct2 result;
	JOYNR_LOG_WARN(logger(), "**********************************************");
	JOYNR_LOG_WARN(logger(), "* DefaultMultipleVersionsInterface2Provider::getInterfaceVersionedStruct called");
	JOYNR_LOG_WARN(logger(), "**********************************************");
	onSuccess(
			result
	);
}


} // namespace tests
} // namespace joynr