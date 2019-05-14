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

#ifndef GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2SYNCPROXY_H
#define GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2SYNCPROXY_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/tests/MultipleVersionsInterface2ProxyBase.h"

#include "joynr/tests/InterfaceVersionedStruct2.h"
#include <cstdint>
#include "joynr/tests/MultipleVersionsTypeCollection/VersionedStruct2.h"
#include <string>
#include "joynr/tests/AnonymousVersionedStruct2.h"

#include <memory>

namespace joynr { namespace tests { 
/**
 * @brief Synchronous proxy for interface MultipleVersionsInterface2
 *
 * @version 2.0
 */
class  MultipleVersionsInterface2SyncProxy :
		virtual public MultipleVersionsInterface2ProxyBase,
		virtual public IMultipleVersionsInterface2Sync
{
public:
	/**
	 * @brief Parameterized constructor
	 * @param connectorFactory The connector factory
	 * @param domain The provider domain
	 * @param qosSettings The quality of service settings
	 */
	MultipleVersionsInterface2SyncProxy(
			std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
			std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
			const std::string& domain,
			const joynr::MessagingQos& qosSettings
	);

	
	/**
	* @brief Synchronous getter for the uInt8Attribute1 attribute.
	*
	* @param result The result that will be returned to the caller.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getUInt8Attribute1(std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos = boost::none)
	override;
	
	
	/**
	* @brief Synchronous getter for the uInt8Attribute2 attribute.
	*
	* @param result The result that will be returned to the caller.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getUInt8Attribute2(std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos = boost::none)
	override;
	
	
	/**
	* @brief Synchronous setter for the uInt8Attribute1 attribute.
	*
	* @param uInt8Attribute1 The value to set.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void setUInt8Attribute1(const std::uint8_t& uInt8Attribute1, boost::optional<joynr::MessagingQos> qos = boost::none)
	override;
	
	/**
	* @brief Synchronous setter for the uInt8Attribute2 attribute.
	*
	* @param uInt8Attribute2 The value to set.
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void setUInt8Attribute2(const std::uint8_t& uInt8Attribute2, boost::optional<joynr::MessagingQos> qos = boost::none)
	override;
	/**
	* @brief Synchronous operation getTrue.
	*
	* @param bool result this is an output parameter
	*        and will be set within function getTrue
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getTrue(
				bool& result,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;
	/**
	* @brief Synchronous operation getVersionedStruct.
	*
	* @param joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2 result this is an output parameter
	*        and will be set within function getVersionedStruct
	* @param joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getVersionedStruct(
				joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& result,
				const joynr::tests::MultipleVersionsTypeCollection::VersionedStruct2& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;
	/**
	* @brief Synchronous operation getAnonymousVersionedStruct.
	*
	* @param joynr::tests::AnonymousVersionedStruct2 result this is an output parameter
	*        and will be set within function getAnonymousVersionedStruct
	* @param joynr::tests::AnonymousVersionedStruct2 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getAnonymousVersionedStruct(
				joynr::tests::AnonymousVersionedStruct2& result,
				const joynr::tests::AnonymousVersionedStruct2& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;
	/**
	* @brief Synchronous operation getInterfaceVersionedStruct.
	*
	* @param joynr::tests::InterfaceVersionedStruct2 result this is an output parameter
	*        and will be set within function getInterfaceVersionedStruct
	* @param joynr::tests::InterfaceVersionedStruct2 input
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
	* @throws JoynrException if the request is not successful
	*/
	void getInterfaceVersionedStruct(
				joynr::tests::InterfaceVersionedStruct2& result,
				const joynr::tests::InterfaceVersionedStruct2& input,
				boost::optional<joynr::MessagingQos> qos = boost::none
	)
	override;

	friend class MultipleVersionsInterface2Proxy;

private:
	DISALLOW_COPY_AND_ASSIGN(MultipleVersionsInterface2SyncProxy);
};

} // namespace tests
} // namespace joynr
#endif // GENERATED_INTERFACE_JOYNR_TESTS_MULTIPLEVERSIONSINTERFACE2SYNCPROXY_H