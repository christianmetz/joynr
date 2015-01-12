package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.JoynrInvocationHandler;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MethodInvocation;
import io.joynr.proxy.invocation.Invocation;
import io.joynr.proxy.invocation.SubscriptionInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.subscription.SubscriptionManager;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

public class ProxyInvocationHandler extends JoynrInvocationHandler {
    private final MessagingQos qosSettings;
    private ConnectorStatus connectorStatus;
    private Lock connectorStatusLock = new ReentrantLock();
    private Condition connectorSuccessfullyFinished = connectorStatusLock.newCondition();
    private DiscoveryQos discoveryQos;
    private ConnectorInvocationHandler connector;
    private RequestReplySender messageSender;
    private RequestReplyDispatcher dispatcher;
    private final String proxyParticipantId;
    private SubscriptionManager subscriptionManager;
    private ConcurrentLinkedQueue<MethodInvocation> queuedRpcList = new ConcurrentLinkedQueue<MethodInvocation>();
    private ConcurrentLinkedQueue<SubscriptionInvocation> queuedSubscriptionInvocationList = new ConcurrentLinkedQueue<SubscriptionInvocation>();
    private String interfaceName;
    private String domain;

    private static final Logger logger = LoggerFactory.getLogger(ProxyInvocationHandler.class);

    // CHECKSTYLE:OFF
    public ProxyInvocationHandler(String domain,
                                  String interfaceName,
                                  String proxyParticipantId,
                                  DiscoveryQos discoveryQos,
                                  MessagingQos messagingQos,
                                  RequestReplySender messageSender,
                                  RequestReplyDispatcher dispatcher,
                                  SubscriptionManager subscriptionManager) {
        // CHECKSTYLE:ON
        this.domain = domain;
        this.proxyParticipantId = proxyParticipantId;
        this.interfaceName = interfaceName;
        this.discoveryQos = discoveryQos;
        this.qosSettings = messagingQos;
        this.messageSender = messageSender;
        this.dispatcher = dispatcher;
        this.subscriptionManager = subscriptionManager;
        this.connectorStatus = ConnectorStatus.ConnectorNotAvailabe;
    }

    /**
     * executeSyncMethod is called whenever a method of the synchronous interface which is provided by the proxy is
     * called. The ProxyInvocationHandler will check the arbitration status before the call is delegated to the
     * connector. If the arbitration is still in progress the synchronous call will block until the arbitration was
     * successful or the timeout elapsed.
     * 
     * @throws Throwable
     * @throws InterruptedException
     * @throws IllegalArgumentException
     * 
     * @see java.lang.reflect.InvocationHandler#invoke(java.lang.Object, java.lang.reflect.Method, java.lang.Object[])
     */
    @CheckForNull
    @Override
    public Object executeSyncMethod(Method method, Object[] args) throws IllegalArgumentException,
                                                                 InterruptedException, Throwable {

        try {
            if (waitForConnectorFinished()) {
                if (connector == null) {
                    throw new IllegalStateException("connector was null although arbitration finished successfully");
                }
                return connector.executeSyncMethod(method, args);
            }
        } catch (JoynrException e) {
            throw e;

        } catch (Throwable e) {
            throw new JoynrException(e);
        }

        throw new JoynrArbitrationException("Arbitration and Connector failed: domain: " + domain + " interface: "
                + interfaceName + " qos: " + discoveryQos + ": Arbitration could not be finished in time.");

    }

    /**
     * Checks the connector status before a method call is executed. Instantly returns True if the connector already
     * finished successfully , otherwise it will block up to the amount of milliseconds defined by the
     * arbitrationTimeout or until the ProxyInvocationHandler is notified about a successful connection.
     * 
     * @return True if the connector was finished successfully in time, False if the connector failed or could not be
     *         finished in time.
     * @throws InterruptedException
     */
    public boolean waitForConnectorFinished() throws InterruptedException {
        connectorStatusLock.lock();
        try {
            if (connectorStatus == ConnectorStatus.ConnectorSuccesful) {
                return true;
            }

            return connectorSuccessfullyFinished.await(discoveryQos.getDiscoveryTimeout(), TimeUnit.MILLISECONDS);

        } finally {
            connectorStatusLock.unlock();
        }

    }

    /**
     * Checks if the connector was set successfully. Returns immediately and does not block until the connector is
     * finished.
     * 
     * @return true if a connector was successfully set.
     */
    public boolean isConnectorReady() {
        connectorStatusLock.lock();
        try {
            if (connectorStatus == ConnectorStatus.ConnectorSuccesful) {
                return true;
            }
            return false;

        } finally {
            connectorStatusLock.unlock();
        }

    }

    private void sendQueuedSubscriptionInvocations() {
        while (true) {
            SubscriptionInvocation currentSubscription = queuedSubscriptionInvocationList.poll();
            if (currentSubscription == null) {
                return;
            }

            try {
                // TODO how to react on failures. Setting the failure state in the future is useless as the future is
                // not passed to the app for subscriptions
                if (currentSubscription instanceof AttributeSubscribeInvocation) {
                    connector.executeSubscriptionMethod((AttributeSubscribeInvocation) currentSubscription);
                } else if (currentSubscription instanceof BroadcastSubscribeInvocation) {
                    connector.executeSubscriptionMethod((BroadcastSubscribeInvocation) currentSubscription);
                } else if (currentSubscription instanceof UnsubscribeInvocation) {
                    connector.executeSubscriptionMethod((UnsubscribeInvocation) currentSubscription);
                }
            } catch (JoynrSendBufferFullException e) {
                currentSubscription.getFuture().onFailure(e);

            } catch (JoynrMessageNotSentException e) {
                currentSubscription.getFuture().onFailure(e);

            } catch (JsonGenerationException e) {
                currentSubscription.getFuture().onFailure(new JoynrException(e));

            } catch (JsonMappingException e) {
                currentSubscription.getFuture().onFailure(new JoynrException(e));

            } catch (IOException e) {
                currentSubscription.getFuture().onFailure(new JoynrException(e));
            }
        }
    }

    private void setFutureErrorState(Invocation invocation, JoynrException e) {
        invocation.getFuture().onFailure(e);
    }

    /**
     * Executes previously queued remote calls. This method is called when arbitration is completed.
     */
    private void sendQueuedInvocations() {
        while (true) {
            MethodInvocation currentRPC = queuedRpcList.poll();
            if (currentRPC == null) {
                return;
            }

            try {
                connector.executeAsyncMethod(currentRPC.getMethod(), currentRPC.getArgs(), currentRPC.getFuture());
            } catch (JoynrSendBufferFullException e) {
                currentRPC.getFuture().onFailure(e);

            } catch (JoynrMessageNotSentException e) {
                currentRPC.getFuture().onFailure(e);

            } catch (JsonGenerationException e) {
                currentRPC.getFuture().onFailure(new JoynrException(e));

            } catch (JsonMappingException e) {
                currentRPC.getFuture().onFailure(new JoynrException(e));

            } catch (IOException e) {
                currentRPC.getFuture().onFailure(new JoynrException(e));
            }

        }

    }

    /**
     * Sets the connector for this ProxyInvocationHandler after the DiscoveryAgent got notified about a successful
     * arbitration. Should be called from the DiscoveryAgent
     * 
     * @param result
     *            from the previously invoked arbitration
     */
    public void createConnector(ArbitrationResult result) {
        connector = ConnectorFactory.create(dispatcher,
                                            subscriptionManager,
                                            messageSender,
                                            proxyParticipantId,
                                            result,
                                            qosSettings);
        connectorStatusLock.lock();
        try {
            connectorStatus = ConnectorStatus.ConnectorSuccesful;
            connectorSuccessfullyFinished.signal();

            if (connector != null) {
                sendQueuedInvocations();
                sendQueuedSubscriptionInvocations();
            }
        } finally {
            connectorStatusLock.unlock();
        }
    }

    @CheckForNull
    @Override
    protected Object executeSubscriptionMethod(Method method, Object[] args) throws IllegalAccessException, Throwable {
        Future<String> future = new Future<String>();
        if (method.getName().startsWith("subscribeTo")) {
            AttributeSubscribeInvocation attributeSubscription = new AttributeSubscribeInvocation(method, args, future);
            // TODO how to react on failures / what to do with the future which is returned from executeSubscriptionMethod
            connectorStatusLock.lock();
            try {
                if (!isConnectorReady()) {
                    // waiting for arbitration -> queue invocation
                    queuedSubscriptionInvocationList.offer(attributeSubscription);
                    //TODO Bug: [Java] subscribeTo<Attribute> does not return correct value in case connector is not available
                    return attributeSubscription.getSubscriptionId();
                }
            } finally {
                connectorStatusLock.unlock();
            }

            try {
                connector.executeSubscriptionMethod(attributeSubscription);
            } catch (JoynrSendBufferFullException e) {
                logger.error("error executing attribute subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(attributeSubscription, e);
            } catch (JoynrMessageNotSentException e) {
                logger.error("error executing attribute subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(attributeSubscription, e);
            } catch (JsonGenerationException e) {
                logger.error("error executing attribute subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(attributeSubscription, new JoynrException(e));
            } catch (JsonMappingException e) {
                logger.error("error executing attribute subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(attributeSubscription, new JoynrException(e));
            } catch (IOException e) {
                logger.error("error executing attribute subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(attributeSubscription, new JoynrException(e));
            }

            return attributeSubscription.getSubscriptionId();
        } else if (method.getName().startsWith("unsubscribeFrom")) {
            return unsubscribe(new UnsubscribeInvocation(method, args, future)).getSubscriptionId();
        } else {
            throw new JoynrIllegalStateException("Called unknown method in subscription interface.");
        }
    }

    @Override
    protected Object executeBroadcastSubscriptionMethod(Method method, Object[] args)
                                                                                     throws JoynrSendBufferFullException,
                                                                                     JoynrMessageNotSentException,
                                                                                     JsonGenerationException,
                                                                                     JsonMappingException, IOException {

        Future<String> future = new Future<String>();
        if (method.getName().startsWith("subscribeTo")) {

            BroadcastSubscribeInvocation broadcastSubscription = new BroadcastSubscribeInvocation(method,
                                                                                                  args,
                                                                                                  new Future<String>());
            connectorStatusLock.lock();
            try {
                if (!isConnectorReady()) {
                    // waiting for arbitration -> queue invocation
                    queuedSubscriptionInvocationList.offer(broadcastSubscription);
                    //TODO Bug: [Java] subscribeTo<Attribute> does not return correct value in case connector is not available
                    return broadcastSubscription.getSubscriptionId();
                }
            } finally {
                connectorStatusLock.unlock();
            }

            try {
                connector.executeSubscriptionMethod(broadcastSubscription);
            } catch (JoynrSendBufferFullException e) {
                logger.error("error executing broadcast subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(broadcastSubscription, e);
            } catch (JoynrMessageNotSentException e) {
                logger.error("error executing broadcast subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(broadcastSubscription, e);
            } catch (JsonGenerationException e) {
                logger.error("error executing broadcast subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(broadcastSubscription, new JoynrException(e));
            } catch (JsonMappingException e) {
                logger.error("error executing broadcast subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(broadcastSubscription, new JoynrException(e));
            } catch (IOException e) {
                logger.error("error executing broadcast subscription: {} : {}", method.getName(), e.getMessage());
                setFutureErrorState(broadcastSubscription, new JoynrException(e));
            }
            return broadcastSubscription.getSubscriptionId();
        } else if (method.getName().startsWith("unsubscribeFrom")) {
            return unsubscribe(new UnsubscribeInvocation(method, args, future)).getSubscriptionId();
        } else {
            throw new JoynrIllegalStateException("Called unknown method in broadcast subscription interface.");
        }
    }

    @Override
    protected <T> Object executeAsyncMethod(Method method, Object[] args) throws IllegalAccessException, Throwable {
        Future<T> future = new Future<T>();
        connectorStatusLock.lock();
        try {
            if (!isConnectorReady()) {
                // waiting for arbitration -> queue invocation
                queuedRpcList.offer(new MethodInvocation(method, args, future));
                return future;
            }
        } finally {
            connectorStatusLock.unlock();
        }

        // arbitration already successfully finished -> send invocation
        return connector.executeAsyncMethod(method, args, future);
    }

    private UnsubscribeInvocation unsubscribe(UnsubscribeInvocation unsubscribeInvocation) {
        subscriptionManager.unregisterSubscription(unsubscribeInvocation.getSubscriptionId());
        connectorStatusLock.lock();
        try {
            if (!isConnectorReady()) {
                // waiting for arbitration -> queue invocation
                queuedSubscriptionInvocationList.offer(unsubscribeInvocation);
                return unsubscribeInvocation;
            }
        } finally {
            connectorStatusLock.unlock();
        }

        try {
            connector.executeSubscriptionMethod(unsubscribeInvocation);
        } catch (JoynrSendBufferFullException e) {
            logger.error("error executing unsubscription: {} : {}",
                         unsubscribeInvocation.getSubscriptionId(),
                         e.getMessage());
            setFutureErrorState(unsubscribeInvocation, e);
        } catch (JoynrMessageNotSentException e) {
            logger.error("error executing unsubscription: {} : {}",
                         unsubscribeInvocation.getSubscriptionId(),
                         e.getMessage());
            setFutureErrorState(unsubscribeInvocation, e);
        } catch (JsonGenerationException e) {
            logger.error("error executing unsubscription: {} : {}",
                         unsubscribeInvocation.getSubscriptionId(),
                         e.getMessage());
            setFutureErrorState(unsubscribeInvocation, new JoynrException(e));
        } catch (JsonMappingException e) {
            logger.error("error executing unsubscription: {} : {}",
                         unsubscribeInvocation.getSubscriptionId(),
                         e.getMessage());
            setFutureErrorState(unsubscribeInvocation, new JoynrException(e));
        } catch (IOException e) {
            logger.error("error executing unsubscription: {} : {}",
                         unsubscribeInvocation.getSubscriptionId(),
                         e.getMessage());
            setFutureErrorState(unsubscribeInvocation, new JoynrException(e));
        }

        return unsubscribeInvocation;
    }
}
