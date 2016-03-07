/*
  * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/Request.h"
#include "joynr/Util.h"

namespace joynr
{

bool isRequestTypeRegistered = Variant::registerType<Request>("joynr.Request");

Request::Request() : requestReplyId(util::createUuid()), methodName(), params(), paramDatatypes()
{
}

bool Request::operator==(const Request& other) const
{
    return requestReplyId == other.getRequestReplyId() && methodName == other.getMethodName() &&
           params == other.getParams() && paramDatatypes == other.paramDatatypes;
}

const std::string& Request::getRequestReplyId() const
{
    return requestReplyId;
}

void Request::setRequestReplyId(std::string requestReplyId)
{
    this->requestReplyId = std::move(requestReplyId);
}

const std::string& Request::getMethodName() const
{
    return methodName;
}

void Request::setMethodName(const std::string& methodName)
{
    this->methodName = methodName;
}

std::vector<Variant> Request::getParams() const
{
    return params;
}

// Set the parameters - called by the json deserializer
void Request::setParams(std::vector<Variant> params)
{
    this->params = std::move(params);
}

void Request::addParam(Variant value, std::string datatype)
{
    this->params.push_back(std::move(value));
    this->paramDatatypes.push_back(std::move(datatype));
}

std::vector<std::string> Request::getParamDatatypes() const
{
    return paramDatatypes;
}

// Set the parameter datatypes - called by the json deserializer
void Request::setParamDatatypes(std::vector<std::string> paramDatatypes)
{
    this->paramDatatypes = std::move(paramDatatypes);
}

} // namespace joynr
