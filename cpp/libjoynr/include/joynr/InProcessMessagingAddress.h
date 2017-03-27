/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
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
 * #L%
 */
#ifndef INPROCESSMESSAGINGADDRESS_H
#define INPROCESSMESSAGINGADDRESS_H

#include <memory>

#include "joynr/JoynrCommonExport.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class InProcessMessagingSkeleton;

class JOYNRCOMMON_EXPORT InProcessMessagingAddress : public joynr::system::RoutingTypes::Address
{
public:
    InProcessMessagingAddress() = default;
    explicit InProcessMessagingAddress(std::shared_ptr<InProcessMessagingSkeleton> skeleton);
    std::shared_ptr<InProcessMessagingSkeleton> getSkeleton() const;

    template <typename Archive>
    void serialize(Archive&)
    {
    }

private:
    std::shared_ptr<InProcessMessagingSkeleton> skeleton;
};

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::InProcessMessagingAddress,
                                 joynr::system::RoutingTypes::Address,
                                 "joynr.InProcessMessagingAddress")

#endif // INPROCESSMESSAGINGADDRESS_H
