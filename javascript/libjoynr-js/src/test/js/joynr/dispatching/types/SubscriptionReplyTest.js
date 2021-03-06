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

const SubscriptionReply = require("joynr/joynr/dispatching/types/SubscriptionReply");
const SubscriptionException = require("joynr/joynr/exceptions/SubscriptionException");

describe("libjoynr-js.joynr.dispatching.types.SubscriptionReply", () => {
    it("is instantiable", () => {
        const subscriptionReply = new SubscriptionReply({
            subscriptionId: "id"
        });
        expect(subscriptionReply).toBeDefined();
        expect(subscriptionReply instanceof SubscriptionReply).toBeTruthy();
        expect(subscriptionReply._typeName).toEqual("joynr.SubscriptionReply");
        expect(subscriptionReply.subscriptionId).toEqual("id");
    });

    it("is instantiable with error", () => {
        const subscriptionException = new SubscriptionException({
            subscriptionId: "id"
        });

        const subscriptionReply = new SubscriptionReply({
            subscriptionId: "id",
            error: subscriptionException
        });
        expect(subscriptionReply).toBeDefined();
        expect(subscriptionReply instanceof SubscriptionReply).toBeTruthy();
        expect(subscriptionReply._typeName).toEqual("joynr.SubscriptionReply");
        expect(subscriptionReply.subscriptionId).toEqual("id");
        expect(subscriptionReply.error).toEqual(subscriptionException);
    });
});
