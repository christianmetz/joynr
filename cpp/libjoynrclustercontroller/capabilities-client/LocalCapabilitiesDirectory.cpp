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

#include "joynr/LocalCapabilitiesDirectory.h"

#include <algorithm>
#include <unordered_set>
#include <ostream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/range/join.hpp>
#include <spdlog/fmt/fmt.h>

#include "joynr/access-control/IAccessController.h"

#include "joynr/CallContextStorage.h"
#include "joynr/CapabilityUtils.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/IMessageRouter.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

#include "libjoynrclustercontroller/capabilities-client/ICapabilitiesClient.h"

namespace joynr
{

struct DiscoveryEntryHash
{
    std::size_t operator()(const types::DiscoveryEntry& entry) const
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, entry.getParticipantId());
        return seed;
    }
};

struct DiscoveryEntryKeyEq
{
    bool operator()(const types::DiscoveryEntry& lhs, const types::DiscoveryEntry& rhs) const
    {
        // there is no need to check typeid because entries are of the same type.
        return joynr::util::compareValues(lhs.getParticipantId(), rhs.getParticipantId());
    }
};

LocalCapabilitiesDirectory::LocalCapabilitiesDirectory(
        ClusterControllerSettings& clusterControllerSettings,
        std::shared_ptr<ICapabilitiesClient> capabilitiesClientPtr,
        const std::string& localAddress,
        std::weak_ptr<IMessageRouter> messageRouter,
        boost::asio::io_service& ioService,
        const std::string clusterControllerId)
        : joynr::system::DiscoveryAbstractProvider(),
          joynr::system::ProviderReregistrationControllerProvider(),
          std::enable_shared_from_this<LocalCapabilitiesDirectory>(),
          clusterControllerSettings(clusterControllerSettings),
          capabilitiesClient(std::move(capabilitiesClientPtr)),
          localAddress(localAddress),
          cacheLock(),
          pendingLookupsLock(),
          messageRouter(messageRouter),
          observers(),
          pendingLookups(),
          accessController(),
          checkExpiredDiscoveryEntriesTimer(ioService),
          isLocalCapabilitiesDirectoryPersistencyEnabled(
                  clusterControllerSettings.isLocalCapabilitiesDirectoryPersistencyEnabled()),
          freshnessUpdateTimer(ioService),
          clusterControllerId(clusterControllerId)
{
}

void LocalCapabilitiesDirectory::init()
{
    scheduleCleanupTimer();
    scheduleFreshnessUpdate();
}

void LocalCapabilitiesDirectory::shutdown()
{
    checkExpiredDiscoveryEntriesTimer.cancel();
    freshnessUpdateTimer.cancel();
}

void LocalCapabilitiesDirectory::scheduleFreshnessUpdate()
{
    boost::system::error_code timerError = boost::system::error_code();
    freshnessUpdateTimer.expires_from_now(
            clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs(), timerError);
    if (timerError) {
        JOYNR_LOG_ERROR(logger(),
                        "Error from freshness update timer: {}: {}",
                        timerError.value(),
                        timerError.message());
    }
    freshnessUpdateTimer.async_wait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
            const boost::system::error_code& timerError) {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            thisSharedPtr->sendAndRescheduleFreshnessUpdate(timerError);
        }
    });
}

void LocalCapabilitiesDirectory::sendAndRescheduleFreshnessUpdate(
        const boost::system::error_code& timerError)
{
    if (timerError == boost::asio::error::operation_aborted) {
        // Assume Destructor has been called
        JOYNR_LOG_DEBUG(logger(),
                        "freshness update aborted after shutdown, error code from freshness update "
                        "timer: {}",
                        timerError.message());
        return;
    } else if (timerError) {
        JOYNR_LOG_ERROR(
                logger(),
                "send freshness update called with error code from freshness update timer: {}",
                timerError.message());
    }

    auto onError = [](const joynr::exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger(), "error sending freshness update: {}", error.getMessage());
    };
    capabilitiesClient->touch(clusterControllerId, nullptr, std::move(onError));
    scheduleFreshnessUpdate();
}

LocalCapabilitiesDirectory::~LocalCapabilitiesDirectory()
{
    freshnessUpdateTimer.cancel();
    checkExpiredDiscoveryEntriesTimer.cancel();
    clear();
}

void LocalCapabilitiesDirectory::addInternal(
        const types::DiscoveryEntry& discoveryEntry,
        bool awaitGlobalRegistration,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    const bool isGloballyVisible = isGlobal(discoveryEntry);

    if (!isGloballyVisible || !awaitGlobalRegistration) {
        // register locally
        insertInLocallyRegisteredCapabilitiesCache(discoveryEntry);
        if (isGloballyVisible) {
            insertInGlobalLookupCache(discoveryEntry);
        }
        // Inform observers
        informObserversOnAdd(discoveryEntry);

        updatePersistedFile();
        {
            std::lock_guard<std::mutex> lock(pendingLookupsLock);
            callPendingLookups(InterfaceAddress(
                    discoveryEntry.getDomain(), discoveryEntry.getInterfaceName()));
        }
    }

    // register globally
    if (isGloballyVisible) {
        types::GlobalDiscoveryEntry globalDiscoveryEntry = toGlobalDiscoveryEntry(discoveryEntry);

        std::function<void(const exceptions::JoynrException&)> onErrorWrapper = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            globalDiscoveryEntry,
            awaitGlobalRegistration,
            onError
        ](const exceptions::JoynrException& error)
        {
            JOYNR_LOG_ERROR(logger(),
                            "Error occurred during the execution of capabilitiesProxy->add for "
                            "'{}'. Error: {}",
                            globalDiscoveryEntry.toString(),
                            error.getMessage());
            if (awaitGlobalRegistration && onError) {
                // no need to remove entry as in this case the entry was not yet added
                onError(exceptions::ProviderRuntimeException(error.getMessage()));
            }
            // in case awaitGlobalRegistration == false, the provider discovery
            // entry will not be deleted, so the provider continues to be available
            // locally, even if this makes little sense except for testing purposes.
            // It will never be informed about the failure to be registered globally
            // since it already got a reply after the local registration succeeded.
        };

        std::function<void()> onSuccessWrapper = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            globalDiscoveryEntry,
            awaitGlobalRegistration,
            onSuccess
        ]()
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                {
                    std::lock_guard<std::mutex> lock(thisSharedPtr->cacheLock);
                    JOYNR_LOG_INFO(logger(),
                                   "Global capability '{}' added successfully, "
                                   "#registeredGlobalCapabilities {}",
                                   globalDiscoveryEntry.toString(),
                                   thisSharedPtr->countGlobalCapabilities());
                }
                if (awaitGlobalRegistration) {
                    thisSharedPtr->insertInLocallyRegisteredCapabilitiesCache(globalDiscoveryEntry);
                    thisSharedPtr->insertInGlobalLookupCache(globalDiscoveryEntry);
                    if (onSuccess) {
                        onSuccess();
                    }

                    // Inform observers
                    thisSharedPtr->informObserversOnAdd(globalDiscoveryEntry);

                    thisSharedPtr->updatePersistedFile();
                    {
                        std::lock_guard<std::mutex> lock(thisSharedPtr->pendingLookupsLock);
                        thisSharedPtr->callPendingLookups(
                                InterfaceAddress(globalDiscoveryEntry.getDomain(),
                                                 globalDiscoveryEntry.getInterfaceName()));
                    }
                }
            }
        };

        capabilitiesClient->add(
                globalDiscoveryEntry, std::move(onSuccessWrapper), std::move(onErrorWrapper));
    }

    if (!isGloballyVisible || !awaitGlobalRegistration) {
        onSuccess();
    }
}

types::GlobalDiscoveryEntry LocalCapabilitiesDirectory::toGlobalDiscoveryEntry(
        const types::DiscoveryEntry& discoveryEntry) const
{
    return types::GlobalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                       discoveryEntry.getDomain(),
                                       discoveryEntry.getInterfaceName(),
                                       discoveryEntry.getParticipantId(),
                                       discoveryEntry.getQos(),
                                       discoveryEntry.getLastSeenDateMs(),
                                       discoveryEntry.getExpiryDateMs(),
                                       discoveryEntry.getPublicKeyId(),
                                       localAddress);
}

void LocalCapabilitiesDirectory::remove(const std::string& participantId,
                                        bool removeGlobally,
                                        bool removeFromGlobalLookupCache)
{
    {
        std::lock_guard<std::mutex> lock(cacheLock);

        boost::optional<types::DiscoveryEntry> optionalEntry =
                locallyRegisteredCapabilities.lookupByParticipantId(participantId);
        if (!optionalEntry) {
            JOYNR_LOG_INFO(
                    logger(), "participantId '{}' not found, cannot be removed", participantId);
            return;
        }
        const types::DiscoveryEntry& entry = *optionalEntry;

        if (isGlobal(entry)) {
            JOYNR_LOG_INFO(
                    logger(), "Removing globally registered participantId: {}", participantId);
            if (removeFromGlobalLookupCache) {
                globalLookupCache.removeByParticipantId(participantId);
            }
            if (removeGlobally) {
                capabilitiesClient->remove(participantId);
            }
            JOYNR_LOG_INFO(logger(),
                           "After removal of participantId {}: #registeredGlobalCapabilities: {}",
                           participantId,
                           countGlobalCapabilities());
        }
        JOYNR_LOG_INFO(logger(),
                       "Removing locally registered participantId: {}, #localCapabilities before "
                       "removal: {}",
                       participantId,
                       locallyRegisteredCapabilities.size());
        locallyRegisteredCapabilities.removeByParticipantId(participantId);
        informObserversOnRemove(entry);

        if (auto messageRouterSharedPtr = messageRouter.lock()) {
            messageRouterSharedPtr->removeNextHop(participantId);
        } else {
            JOYNR_LOG_FATAL(logger(),
                            "could not removeNextHop for {} because messageRouter is not available",
                            participantId);
        }
    }
    updatePersistedFile();
}

void LocalCapabilitiesDirectory::triggerGlobalProviderReregistration(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;

    {
        std::lock_guard<std::mutex> lock(cacheLock);
        for (const auto& capability : locallyRegisteredCapabilities) {
            if (capability.getQos().getScope() == types::ProviderScope::GLOBAL) {
                capabilitiesClient->add(toGlobalDiscoveryEntry(capability), nullptr, nullptr);
            }
        }
    }

    onSuccess();
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedGlobalDiscoveryEntries()
        const
{
    std::lock_guard<std::mutex> lock(cacheLock);

    return std::vector<types::DiscoveryEntry>(globalLookupCache.cbegin(), globalLookupCache.cend());
}

std::size_t LocalCapabilitiesDirectory::countGlobalCapabilities() const
{
    std::size_t counter = 0;
    for (const auto& capability : locallyRegisteredCapabilities) {
        if (capability.getQos().getScope() == types::ProviderScope::GLOBAL) {
            counter++;
        }
    }
    return counter;
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const joynr::types::DiscoveryQos& discoveryQos,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    std::vector<types::DiscoveryEntry> localCapabilities =
            searchCache(interfaceAddresses, std::chrono::milliseconds(-1), true);
    std::vector<types::DiscoveryEntry> globalCapabilities = searchCache(
            interfaceAddresses, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()), false);

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(globalCapabilities),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectory::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    boost::optional<types::DiscoveryEntry> globalCapability =
            searchCache(participantId, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));

    return callReceiverIfPossible(scope,
                                  getCachedLocalCapabilities(participantId),
                                  optionalToVector(std::move(globalCapability)),
                                  std::move(callback));
}

std::vector<types::DiscoveryEntryWithMetaInfo> LocalCapabilitiesDirectory::filterDuplicates(
        std::vector<types::DiscoveryEntryWithMetaInfo>&& localCapabilitiesWithMetaInfo,
        std::vector<types::DiscoveryEntryWithMetaInfo>&& globalCapabilitiesWithMetaInfo)
{
    // use custom DiscoveryEntryHash and custom DiscoveryEntryKeyEq to compare only the
    // participantId and to ignore the isLocal flag of DiscoveryEntryWithMetaInfo.
    // prefer local entries if there are local and global entries for the same provider.
    std::unordered_set<types::DiscoveryEntryWithMetaInfo,
                       joynr::DiscoveryEntryHash,
                       joynr::DiscoveryEntryKeyEq>
            resultSet(std::make_move_iterator(localCapabilitiesWithMetaInfo.begin()),
                      std::make_move_iterator(localCapabilitiesWithMetaInfo.end()));
    resultSet.insert(std::make_move_iterator(globalCapabilitiesWithMetaInfo.begin()),
                     std::make_move_iterator(globalCapabilitiesWithMetaInfo.end()));
    std::vector<types::DiscoveryEntryWithMetaInfo> resultVec(resultSet.begin(), resultSet.end());
    return resultVec;
}

bool LocalCapabilitiesDirectory::callReceiverIfPossible(
        joynr::types::DiscoveryScope::Enum& scope,
        std::vector<types::DiscoveryEntry>&& localCapabilities,
        std::vector<types::DiscoveryEntry>&& globalCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // return only local capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_ONLY) {
        std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                util::convert(true, localCapabilities);
        callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
        return true;
    }

    // return local then global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                util::convert(true, localCapabilities);
        std::vector<types::DiscoveryEntryWithMetaInfo> globalCapabilitiesWithMetaInfo =
                util::convert(false, globalCapabilities);
        if (!localCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
            return true;
        }
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(globalCapabilitiesWithMetaInfo));
            return true;
        }
    }

    // return local and global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        if (!globalCapabilities.empty()) {
            std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                    util::convert(true, localCapabilities);
            std::vector<types::DiscoveryEntryWithMetaInfo> globalCapabilitiesWithMetaInfo =
                    util::convert(false, globalCapabilities);

            // remove duplicates
            std::vector<types::DiscoveryEntryWithMetaInfo> resultVec =
                    filterDuplicates(std::move(localCapabilitiesWithMetaInfo),
                                     std::move(globalCapabilitiesWithMetaInfo));
            callback->capabilitiesReceived(std::move(resultVec));
            return true;
        }
    }

    // return the global cached entries
    if (scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        std::vector<types::DiscoveryEntryWithMetaInfo> resultWithDuplicates =
                util::convert(false, globalCapabilities);
        std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
                util::convert(true, localCapabilities);
        for (const auto& entry : localCapabilitiesWithMetaInfo) {
            if (entry.getQos().getScope() == joynr::types::ProviderScope::GLOBAL) {
                resultWithDuplicates.push_back(entry);
            }
        }
        if (!resultWithDuplicates.empty()) {
            // remove duplicates
            std::unordered_set<types::DiscoveryEntryWithMetaInfo,
                               joynr::DiscoveryEntryHash,
                               joynr::DiscoveryEntryKeyEq>
                    resultSet(std::make_move_iterator(resultWithDuplicates.begin()),
                              std::make_move_iterator(resultWithDuplicates.end()));
            std::vector<types::DiscoveryEntryWithMetaInfo> result(
                    resultSet.begin(), resultSet.end());
            callback->capabilitiesReceived(std::move(result));
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::capabilitiesReceived(
        const std::vector<types::GlobalDiscoveryEntry>& results,
        std::vector<types::DiscoveryEntry>&& localEntries,
        std::shared_ptr<ILocalCapabilitiesCallback> callback,
        joynr::types::DiscoveryScope::Enum discoveryScope)
{
    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    std::vector<types::DiscoveryEntryWithMetaInfo> globalEntries;

    for (types::GlobalDiscoveryEntry globalDiscoveryEntry : results) {
        types::DiscoveryEntryWithMetaInfo convertedEntry =
                util::convert(false, globalDiscoveryEntry);
        capabilitiesMap.insert(
                {globalDiscoveryEntry.getAddress(), std::move(globalDiscoveryEntry)});
        globalEntries.push_back(std::move(convertedEntry));
    }
    registerReceivedCapabilities(std::move(capabilitiesMap));

    if (discoveryScope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryScope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        std::vector<types::DiscoveryEntryWithMetaInfo> localEntriesWithMetaInfo =
                util::convert(true, localEntries);
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        globalEntries =
                filterDuplicates(std::move(localEntriesWithMetaInfo), std::move(globalEntries));
    }
    callback->capabilitiesReceived(std::move(globalEntries));
}

void LocalCapabilitiesDirectory::lookup(const std::string& participantId,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                                        bool useGlobalCapabilitiesDirectory)
{
    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(participantId, discoveryQos, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        if (!useGlobalCapabilitiesDirectory) {
            // invoke error on callback as no local capabilities were found
            callback->onError(joynr::exceptions::JoynrRuntimeException(fmt::format(
                    "No local capabilities found for participantId {}", participantId)));
            return;
        }

        // search for global entires in the global capabilities directory
        auto onSuccess = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            participantId,
            callback
        ](const std::vector<joynr::types::GlobalDiscoveryEntry>& result)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                thisSharedPtr->capabilitiesReceived(
                        result,
                        thisSharedPtr->getCachedLocalCapabilities(participantId),
                        callback,
                        joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
            }
        };
        capabilitiesClient->lookup(participantId,
                                   std::move(onSuccess),
                                   std::bind(&ILocalCapabilitiesCallback::onError,
                                             std::move(callback),
                                             std::placeholders::_1));
    }
}

void LocalCapabilitiesDirectory::lookup(const std::vector<std::string>& domains,
                                        const std::string& interfaceName,
                                        std::shared_ptr<ILocalCapabilitiesCallback> callback,
                                        const joynr::types::DiscoveryQos& discoveryQos)
{
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.reserve(domains.size());
    for (std::size_t i = 0; i < domains.size(); i++) {
        interfaceAddresses.push_back(InterfaceAddress(domains.at(i), interfaceName));
    }

    // get the local and cached entries
    bool receiverCalled = getLocalAndCachedCapabilities(interfaceAddresses, discoveryQos, callback);

    // if no receiver is called, use the global capabilities directory
    if (!receiverCalled) {
        // search for global entires in the global capabilities directory
        auto onSuccess = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            interfaceAddresses,
            callback,
            discoveryQos
        ](std::vector<joynr::types::GlobalDiscoveryEntry> capabilities)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::mutex> lock(thisSharedPtr->pendingLookupsLock);
                if (!(thisSharedPtr->isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    thisSharedPtr->capabilitiesReceived(
                            capabilities,
                            thisSharedPtr->getCachedLocalCapabilities(interfaceAddresses),
                            callback,
                            discoveryQos.getDiscoveryScope());
                }
                thisSharedPtr->callbackCalled(interfaceAddresses, callback);
            }
        };

        auto onError = [
            thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
            interfaceAddresses,
            callback,
            discoveryQos
        ](const exceptions::JoynrRuntimeException& error)
        {
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                std::lock_guard<std::mutex> lock(thisSharedPtr->pendingLookupsLock);
                if (!(thisSharedPtr->isCallbackCalled(
                            interfaceAddresses, callback, discoveryQos))) {
                    callback->onError(error);
                }
                thisSharedPtr->callbackCalled(interfaceAddresses, callback);
            }
        };

        if (discoveryQos.getDiscoveryScope() == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
            std::lock_guard<std::mutex> lock(pendingLookupsLock);
            registerPendingLookup(interfaceAddresses, callback);
        }
        capabilitiesClient->lookup(domains,
                                   interfaceName,
                                   discoveryQos.getDiscoveryTimeout(),
                                   std::move(onSuccess),
                                   std::move(onError));
    }
}

void LocalCapabilitiesDirectory::callPendingLookups(const InterfaceAddress& interfaceAddress)
{
    if (pendingLookups.find(interfaceAddress) == pendingLookups.cend()) {
        return;
    }
    std::vector<types::DiscoveryEntry> localCapabilities =
            searchCache({interfaceAddress}, std::chrono::milliseconds(-1), true);
    if (localCapabilities.empty()) {
        return;
    }
    std::vector<types::DiscoveryEntryWithMetaInfo> localCapabilitiesWithMetaInfo =
            util::convert(true, localCapabilities);

    for (const std::shared_ptr<ILocalCapabilitiesCallback>& callback :
         pendingLookups[interfaceAddress]) {
        callback->capabilitiesReceived(localCapabilitiesWithMetaInfo);
    }
    pendingLookups.erase(interfaceAddress);
}

void LocalCapabilitiesDirectory::registerPendingLookup(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        pendingLookups[address].push_back(callback); // if no entry exists for key address, an
                                                     // empty list is automatically created
    }
}

bool LocalCapabilitiesDirectory::hasPendingLookups()
{
    return !pendingLookups.empty();
}

bool LocalCapabilitiesDirectory::isCallbackCalled(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback,
        const joynr::types::DiscoveryQos& discoveryQos)
{
    // only if discovery scope is joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL, the
    // callback can potentially already be called, as a matching capability has been added
    // to the local capabilities directory while waiting for capabilitiesclient->lookup result
    if (discoveryQos.getDiscoveryScope() != joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        return false;
    }
    for (const InterfaceAddress& address : interfaceAddresses) {
        if (pendingLookups.find(address) == pendingLookups.cend()) {
            return true;
        }
        if (std::find(pendingLookups[address].cbegin(), pendingLookups[address].cend(), callback) ==
            pendingLookups[address].cend()) {
            return true;
        }
    }
    return false;
}

void LocalCapabilitiesDirectory::callbackCalled(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        if (pendingLookups.find(address) != pendingLookups.cend()) {
            std::vector<std::shared_ptr<ILocalCapabilitiesCallback>>& callbacks =
                    pendingLookups[address];
            util::removeAll(callbacks, callback);
            if (pendingLookups[address].empty()) {
                pendingLookups.erase(address);
            }
        }
    }
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::string& participantId)
{
    std::lock_guard<std::mutex> lock(cacheLock);
    return optionalToVector(locallyRegisteredCapabilities.lookupByParticipantId(participantId));
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::getCachedLocalCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    return searchCache(interfaceAddresses, std::chrono::milliseconds(-1), true);
}

void LocalCapabilitiesDirectory::clear()
{
    std::lock_guard<std::mutex> lock(cacheLock);
    locallyRegisteredCapabilities.clear();
    globalLookupCache.clear();
}

void LocalCapabilitiesDirectory::registerReceivedCapabilities(
        const std::unordered_multimap<std::string, types::DiscoveryEntry>&& capabilityEntries)
{
    for (auto it = capabilityEntries.cbegin(); it != capabilityEntries.cend(); ++it) {
        const std::string& serializedAddress = it->first;
        std::shared_ptr<const system::RoutingTypes::Address> address;
        try {
            joynr::serializer::deserializeFromJson(address, serializedAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger(),
                            "could not deserialize Address from {} - error: {}",
                            serializedAddress,
                            e.what());
            continue;
        }

        const types::DiscoveryEntry& currentEntry = it->second;
        const bool isGloballyVisible = isGlobal(currentEntry);
        if (auto messageRouterSharedPtr = messageRouter.lock()) {
            constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
            const bool isSticky = false;
            messageRouterSharedPtr->addNextHop(currentEntry.getParticipantId(),
                                               address,
                                               isGloballyVisible,
                                               expiryDateMs,
                                               isSticky);
        } else {
            JOYNR_LOG_FATAL(logger(),
                            "could not addNextHop {} to {} because messageRouter is not available",
                            currentEntry.getParticipantId(),
                            serializedAddress);
            return;
        }
        insertInGlobalLookupCache(currentEntry);
    }
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    const bool awaitGlobalRegistration = false;
    return add(discoveryEntry, awaitGlobalRegistration, std::move(onSuccess), std::move(onError));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::add(
        const types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (hasProviderPermission(discoveryEntry)) {
        addInternal(
                discoveryEntry, awaitGlobalRegistration, std::move(onSuccess), std::move(onError));
        return;
    }
    onError(joynr::exceptions::ProviderRuntimeException(
            fmt::format("Provider does not have permissions to register interface {} on domain {}.",
                        discoveryEntry.getInterfaceName(),
                        discoveryEntry.getDomain())));
}

bool LocalCapabilitiesDirectory::hasProviderPermission(const types::DiscoveryEntry& discoveryEntry)
{
    if (!clusterControllerSettings.enableAccessController()) {
        return true;
    }

    if (auto gotAccessController = accessController.lock()) {
        const CallContext& callContext = CallContextStorage::get();
        const std::string& ownerId = callContext.getPrincipal();
        JOYNR_LOG_TRACE(logger(), "hasProviderPermission for ownerId={}", ownerId);
        const bool result = gotAccessController->hasProviderPermission(
                ownerId,
                infrastructure::DacTypes::TrustLevel::HIGH,
                discoveryEntry.getDomain(),
                discoveryEntry.getInterfaceName());
        if (clusterControllerSettings.aclAudit()) {
            if (!result) {
                JOYNR_LOG_ERROR(logger(),
                                "ACL AUDIT: owner '{}' is not allowed to register "
                                "interface '{}' on domain '{}'",
                                ownerId,
                                discoveryEntry.getInterfaceName(),
                                discoveryEntry.getDomain());
            } else {
                JOYNR_LOG_DEBUG(logger(),
                                "ACL AUDIT: owner '{}' is allowed to register interface "
                                "'{}' on domain '{}'",
                                ownerId,
                                discoveryEntry.getInterfaceName(),
                                discoveryEntry.getDomain());
            }
            return true;
        }
        return result;
    }

    // return false in case AC ptr and setting do not match
    return false;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::optionalToVector(
        boost::optional<types::DiscoveryEntry> optionalEntry)
{
    std::vector<types::DiscoveryEntry> vec;
    if (optionalEntry) {
        vec.push_back(std::move(*optionalEntry));
    }
    return vec;
}

void LocalCapabilitiesDirectory::setAccessController(
        std::weak_ptr<IAccessController> accessController)
{
    this->accessController = std::move(accessController);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>& result)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    if (domains.size() != 1) {
        onError(joynr::exceptions::ProviderRuntimeException(
                "LocalCapabilitiesDirectory does not yet support lookup on multiple domains."));
        return;
    }

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(onSuccess), std::move(onError));

    lookup(domains, interfaceName, std::move(localCapabilitiesCallback), discoveryQos);
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::lookup(
        const std::string& participantId,
        std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    auto callback = [
        thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this()),
        onSuccess = std::move(onSuccess),
        onError,
        participantId
    ](const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            if (capabilities.size() == 0) {
                joynr::exceptions::ProviderRuntimeException exception(
                        "No capabilities found for participandId \"" + participantId + "\"");
                onError(exception);
                return;
            }
            if (capabilities.size() > 1) {
                JOYNR_LOG_ERROR(thisSharedPtr->logger(),
                                "participantId {} has more than 1 capability entry:\n {}\n {}",
                                participantId,
                                capabilities[0].toString(),
                                capabilities[1].toString());
            }

            onSuccess(capabilities[0]);
        }
    };

    auto localCapabilitiesCallback =
            std::make_shared<LocalCapabilitiesCallback>(std::move(callback), std::move(onError));
    lookup(participantId, std::move(localCapabilitiesCallback));
}

// inherited method from joynr::system::DiscoveryProvider
void LocalCapabilitiesDirectory::remove(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    remove(participantId);
    onSuccess();
}

void LocalCapabilitiesDirectory::addProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    observers.push_back(std::move(observer));
}

void LocalCapabilitiesDirectory::removeProviderRegistrationObserver(
        std::shared_ptr<LocalCapabilitiesDirectory::IProviderRegistrationObserver> observer)
{
    util::removeAll(observers, observer);
}

void LocalCapabilitiesDirectory::updatePersistedFile()
{
    saveLocalCapabilitiesToFile(
            clusterControllerSettings.getLocalCapabilitiesDirectoryPersistenceFilename());
}

void LocalCapabilitiesDirectory::saveLocalCapabilitiesToFile(const std::string& fileName)
{
    if (!isLocalCapabilitiesDirectoryPersistencyEnabled) {
        return;
    }

    if (fileName.empty()) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(cacheLock);
        joynr::util::saveStringToFile(
                fileName, joynr::serializer::serializeToJson(locallyRegisteredCapabilities));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }
}

void LocalCapabilitiesDirectory::loadPersistedFile()
{
    if (!isLocalCapabilitiesDirectoryPersistencyEnabled) {
        return;
    }

    const std::string persistencyFile =
            clusterControllerSettings.getLocalCapabilitiesDirectoryPersistenceFilename();

    if (persistencyFile.empty()) { // Persistency disabled
        return;
    }

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(persistencyFile);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger(), ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(cacheLock);

    try {
        joynr::serializer::deserializeFromJson(locallyRegisteredCapabilities, jsonString);
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }

    // insert all global capability entries into global cache
    for (const auto& entry : locallyRegisteredCapabilities) {
        if (entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
            globalLookupCache.insert(entry);
        }
    }
}

void LocalCapabilitiesDirectory::injectGlobalCapabilitiesFromFile(const std::string& fileName)
{
    if (fileName.empty()) {
        JOYNR_LOG_WARN(
                logger(), "Empty file name provided in input: cannot load global capabilities.");
        return;
    }

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(fileName);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    std::vector<joynr::types::GlobalDiscoveryEntry> injectedGlobalCapabilities;
    try {
        joynr::serializer::deserializeFromJson(injectedGlobalCapabilities, jsonString);
    } catch (const std::invalid_argument& e) {
        std::string errorMessage("could not deserialize injected global capabilities from " +
                                 jsonString + " - error: " + e.what());
        JOYNR_LOG_FATAL(logger(), errorMessage);
        return;
    }

    if (injectedGlobalCapabilities.empty()) {
        return;
    }

    std::unordered_multimap<std::string, types::DiscoveryEntry> capabilitiesMap;
    for (const auto& globalDiscoveryEntry : injectedGlobalCapabilities) {
        // insert in map for messagerouter
        capabilitiesMap.insert(
                {globalDiscoveryEntry.getAddress(), std::move(globalDiscoveryEntry)});
    }

    // insert found capabilities in messageRouter
    registerReceivedCapabilities(std::move(capabilitiesMap));
}

/**
 * Private convenience methods.
 */
void LocalCapabilitiesDirectory::insertInLocallyRegisteredCapabilitiesCache(
        const types::DiscoveryEntry& entry)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    locallyRegisteredCapabilities.insert(entry);
    JOYNR_LOG_INFO(logger(),
                   "Added local capability to cache {}, #localCapabilities: {}",
                   entry.toString(),
                   locallyRegisteredCapabilities.size());
}

/**
 * Private convenience methods.
 */
void LocalCapabilitiesDirectory::insertInGlobalLookupCache(const types::DiscoveryEntry& entry)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    globalLookupCache.insert(entry);
    JOYNR_LOG_INFO(logger(),
                   "Added global capability to cache {}, #globalLookupCache: {}",
                   entry.toString(),
                   globalLookupCache.size());
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCache(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        std::chrono::milliseconds maxCacheAge,
        bool localEntries)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    std::vector<types::DiscoveryEntry> result;
    for (std::size_t i = 0; i < interfaceAddresses.size(); i++) {
        const InterfaceAddress& interfaceAddress = interfaceAddresses.at(i);
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        std::vector<types::DiscoveryEntry> entries =
                localEntries
                        ? locallyRegisteredCapabilities.lookupByDomainAndInterface(
                                  domain, interface)
                        : globalLookupCache.lookupCacheByDomainAndInterface(
                                  domain, interface, maxCacheAge);
        result.insert(result.end(),
                      std::make_move_iterator(entries.begin()),
                      std::make_move_iterator(entries.end()));
    }
    return result;
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectory::searchCache(
        const std::string& participantId,
        std::chrono::milliseconds maxCacheAge)
{
    std::lock_guard<std::mutex> lock(cacheLock);

    // first search locally
    auto entry = locallyRegisteredCapabilities.lookupByParticipantId(participantId);
    if (!entry) {
        if (maxCacheAge == std::chrono::milliseconds(-1)) {
            entry = globalLookupCache.lookupByParticipantId(participantId);
        } else {
            entry = globalLookupCache.lookupCacheByParticipantId(participantId, maxCacheAge);
        }
    }
    return entry;
}

void LocalCapabilitiesDirectory::informObserversOnAdd(const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : observers) {
        observer->onProviderAdd(discoveryEntry);
    }
}

void LocalCapabilitiesDirectory::informObserversOnRemove(
        const types::DiscoveryEntry& discoveryEntry)
{
    for (const std::shared_ptr<IProviderRegistrationObserver>& observer : observers) {
        observer->onProviderRemove(discoveryEntry);
    }
}

bool LocalCapabilitiesDirectory::isGlobal(const types::DiscoveryEntry& discoveryEntry) const
{
    return discoveryEntry.getQos().getScope() == types::ProviderScope::GLOBAL;
}

void LocalCapabilitiesDirectory::scheduleCleanupTimer()
{
    boost::system::error_code timerError;
    auto intervalMs = clusterControllerSettings.getPurgeExpiredDiscoveryEntriesIntervalMs();
    checkExpiredDiscoveryEntriesTimer.expires_from_now(
            std::chrono::milliseconds(intervalMs), timerError);
    if (timerError) {
        JOYNR_LOG_FATAL(logger(),
                        "Error scheduling discovery entries check. {}: {}",
                        timerError.value(),
                        timerError.message());
    } else {
        checkExpiredDiscoveryEntriesTimer
                .async_wait([thisWeakPtr = joynr::util::as_weak_ptr(shared_from_this())](
                        const boost::system::error_code& errorCode) {
                    if (auto thisSharedPtr = thisWeakPtr.lock()) {
                        thisSharedPtr->checkExpiredDiscoveryEntries(errorCode);
                    }
                });
    }
}

void LocalCapabilitiesDirectory::checkExpiredDiscoveryEntries(
        const boost::system::error_code& errorCode)
{
    if (errorCode == boost::asio::error::operation_aborted) {
        // Assume Destructor has been called
        JOYNR_LOG_DEBUG(logger(),
                        "expired discovery entries check aborted after shutdown, error code from "
                        "expired discovery entries timer: {}",
                        errorCode.message());
        return;
    } else if (errorCode) {
        JOYNR_LOG_ERROR(logger(),
                        "Error triggering expired discovery entries check, error code: {}",
                        errorCode.message());
    }

    bool fileUpdateRequired = false;
    {
        std::lock_guard<std::mutex> lock(cacheLock);

        auto removedLocalCapabilities = locallyRegisteredCapabilities.removeExpired();
        auto removedGlobalCapabilities = globalLookupCache.removeExpired();

        if (!removedLocalCapabilities.empty() || !removedGlobalCapabilities.empty()) {
            fileUpdateRequired = true;
            if (auto messageRouterSharedPtr = messageRouter.lock()) {
                JOYNR_LOG_INFO(logger(),
                               "Following discovery entries expired: local: {}, "
                               "#localCapabilities: {}, global: {}, #globalLookupCache: {}",
                               joinToString(removedLocalCapabilities),
                               locallyRegisteredCapabilities.size(),
                               joinToString(removedGlobalCapabilities),
                               globalLookupCache.size());

                for (const auto& capability :
                     boost::join(removedLocalCapabilities, removedGlobalCapabilities)) {
                    messageRouterSharedPtr->removeNextHop(capability.getParticipantId());
                }
            } else {
                JOYNR_LOG_FATAL(logger(),
                                "could not call removeNextHop because messageRouter is "
                                "not available");
            }
        }
    }

    if (fileUpdateRequired) {
        updatePersistedFile();
    }

    scheduleCleanupTimer();
}

std::string LocalCapabilitiesDirectory::joinToString(
        const std::vector<types::DiscoveryEntry>& discoveryEntries) const
{
    std::ostringstream outputStream;

    std::transform(
            discoveryEntries.cbegin(),
            discoveryEntries.cend(),
            std::ostream_iterator<std::string>(outputStream, ", "),
            [](const types::DiscoveryEntry& discoveryEntry) { return discoveryEntry.toString(); });

    return outputStream.str();
}

LocalCapabilitiesCallback::LocalCapabilitiesCallback(
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)>&& onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)>&& onError)
        : onSuccess(std::move(onSuccess)), onErrorCallback(std::move(onError))
{
}

void LocalCapabilitiesCallback::onError(const exceptions::JoynrRuntimeException& error)
{
    onErrorCallback(joynr::exceptions::ProviderRuntimeException(
            "Unable to collect capabilities from global capabilities directory. Error: " +
            error.getMessage()));
}

void LocalCapabilitiesCallback::capabilitiesReceived(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& capabilities)
{
    onSuccess(capabilities);
}

} // namespace joynr
