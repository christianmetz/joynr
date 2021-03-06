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
package io.joynr.messaging.routing;

import joynr.system.RoutingTypes.Address;

public interface RoutingTable {
    Address get(String participantId);

    /**
     * Adds a new routing entry. If a routing entry for the provided participantId already exists, only the expiryDate
     * and the sticky-flag are updated unless the update is allowed (See RoutingTableAddressValidator).
     *
     * @param participantId participant id for which a routing entry shall be created
     * @param address Address which shall be associated with the participant id
     * @param isGloballyVisible States whether the endpoint is globally visible or not
     * @param expiryDateMs Expiry date of the routing entry in milliseconds
     * @param isSticky If set to true, the routing entry never expires and cannot be replaced
     */
    void put(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs, boolean isSticky);

    /**
     * Overload of put method with isSticky set to false.
     */
    void put(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs);

    boolean containsKey(String participantId);

    /**
     * Query the routing table for the status of isGloballyVisible parameter
     * @param participantId
     * @return true if participantId is globally visible,
     *         false if participantId is not globally visible
     * @throws JoynrRuntimeException if no entry exists for the given participantId
     */
    boolean getIsGloballyVisible(String participantId);

    /**
     * Query the expiry date of a routing entry for a participant id.
     * @param participantId
     * @return The routing entry's expiry date in ms.
     */
    long getExpiryDateMs(String participantId);

    /**
     * Query the sticky-flag of a routing entry for a participant id.
     * @param participantId
     * @return The routing entry's sticky state.
     */
    boolean getIsSticky(String participantId);

    void remove(String participantId);

    /**
     * Apply the specified operation to all addresses currently held in the routing table.
     *
     * @param addressOperation
     *            the address operation to perform.
     */
    void apply(AddressOperation addressOperation);

    /**
     * Purge all expired routing entries from the table
     */
    void purge();
}
