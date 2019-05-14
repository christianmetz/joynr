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

#include "joynr/tests/v2/MultipleVersionsInterfaceSyncProxy.h"

#include <cstdint>
#include "joynr/tests/v2/AnonymousVersionedStruct.h"
#include "joynr/tests/v2/MultipleVersionsTypeCollection/VersionedStruct.h"
#include "joynr/tests/v2/InterfaceVersionedStruct.h"

namespace joynr { namespace tests { namespace v2 { 
// The proxies will contain all arbitration checks
// the connectors will contain the JSON related code

MultipleVersionsInterfaceSyncProxy::MultipleVersionsInterfaceSyncProxy(
		std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
		std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
		joynr::ProxyBase(runtime, connectorFactory, domain, qosSettings),
		MultipleVersionsInterfaceProxyBase(runtime, connectorFactory, domain, qosSettings)
{
}

void MultipleVersionsInterfaceSyncProxy::getUInt8Attribute1(std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getUInt8Attribute1 because the required runtime has been already destroyed.";
		}
		else {
			errorMsg = "proxy cannot invoke getUInt8Attribute1 because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getUInt8Attribute1(uInt8Attribute1, std::move(qos));
	}
}
void MultipleVersionsInterfaceSyncProxy::setUInt8Attribute1(const std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke setUInt8Attribute1 because the required runtime has been already destroyed.";
		}
		else {
			errorMsg = "proxy cannot invoke setUInt8Attribute1 because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->setUInt8Attribute1(uInt8Attribute1, std::move(qos));
	}
}

void MultipleVersionsInterfaceSyncProxy::getUInt8Attribute2(std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getUInt8Attribute2 because the required runtime has been already destroyed.";
		}
		else {
			errorMsg = "proxy cannot invoke getUInt8Attribute2 because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getUInt8Attribute2(uInt8Attribute2, std::move(qos));
	}
}
void MultipleVersionsInterfaceSyncProxy::setUInt8Attribute2(const std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke setUInt8Attribute2 because the required runtime has been already destroyed.";
		}
		else {
			errorMsg = "proxy cannot invoke setUInt8Attribute2 because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->setUInt8Attribute2(uInt8Attribute2, std::move(qos));
	}
}

/*
 * getTrue
 */
void MultipleVersionsInterfaceSyncProxy::getTrue(
			bool& result,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getTrue because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getTrue because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getTrue(result,  std::move(qos));
	}
}
/*
 * getVersionedStruct
 */
void MultipleVersionsInterfaceSyncProxy::getVersionedStruct(
			joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct& result,
			const joynr::tests::v2::MultipleVersionsTypeCollection::VersionedStruct& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getVersionedStruct because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getVersionedStruct because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getVersionedStruct(result, input, std::move(qos));
	}
}
/*
 * getAnonymousVersionedStruct
 */
void MultipleVersionsInterfaceSyncProxy::getAnonymousVersionedStruct(
			joynr::tests::v2::AnonymousVersionedStruct& result,
			const joynr::tests::v2::AnonymousVersionedStruct& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getAnonymousVersionedStruct because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getAnonymousVersionedStruct because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getAnonymousVersionedStruct(result, input, std::move(qos));
	}
}
/*
 * getInterfaceVersionedStruct
 */
void MultipleVersionsInterfaceSyncProxy::getInterfaceVersionedStruct(
			joynr::tests::v2::InterfaceVersionedStruct& result,
			const joynr::tests::v2::InterfaceVersionedStruct& input,
			boost::optional<joynr::MessagingQos> qos
)
{
	auto runtimeSharedPtr = runtime.lock();
	if (!runtimeSharedPtr || (connector==nullptr)) {
		std::string errorMsg;
		if (!runtimeSharedPtr) {
			errorMsg = "proxy cannot invoke getInterfaceVersionedStruct because the required runtime has been already destroyed.";
		}
		if (connector==nullptr){
			errorMsg = "proxy cannot invoke getInterfaceVersionedStruct because the communication end partner is not (yet) known";
		}
		JOYNR_LOG_WARN(logger(), errorMsg);
		exceptions::JoynrRuntimeException error(errorMsg);
		throw error;
	}
	else{
		return connector->getInterfaceVersionedStruct(result, input, std::move(qos));
	}
}

} // namespace v2
} // namespace tests
} // namespace joynr