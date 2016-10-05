/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define(
        "joynr/messaging/mqtt/MqttMessageReplyToAddressCalculator",
        [
            "joynr/messaging/JoynrMessage",
            "joynr/util/JSONSerializer"
        ],
        function(JoynrMessage, JSONSerializer) {

            /**
             * @name MqttMessageReplyToAddressCalculator
             * @constructor
             * @param {Object} settings the settings object for this constructor call
             * @param {MqttAddress} settings.replyToMqttAddress the mqtt address the reply should be send to
             */
            function MqttMessageReplyToAddressCalculator(settings) {
                var replyToAddress = JSONSerializer.stringify(settings.replyToMqttAddress);

                this.setReplyTo =
                        function(message) {
                            var type = message.type;
                            if ((type !== undefined)
                                && (message.replyChannelId === undefined)
                                && ((type === JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST)
                                    || (type === JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                                    || (type === JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) || (type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST))) {
                                message.replyChannelId = replyToAddress;
                            }
                        };
            }

            return MqttMessageReplyToAddressCalculator;

        });