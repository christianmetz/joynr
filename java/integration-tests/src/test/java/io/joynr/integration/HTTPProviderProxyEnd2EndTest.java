package io.joynr.integration;

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

import java.util.Properties;

import com.google.inject.Module;
import com.google.inject.util.Modules;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import org.eclipse.jetty.server.Server;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Ignore;

@Ignore("HTTP does not support binary messages (SMRF)")
public class HTTPProviderProxyEnd2EndTest extends AbstractProviderProxyEnd2EndTest {

    private static Server jettyServer;
    private static Properties originalProperties;

    @BeforeClass
    public static void startServer() throws Exception {
        originalProperties = System.getProperties();
        System.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION, "true");
        System.setProperties(baseTestConfig);

        provisionDiscoveryDirectoryAccessControlEntries();
        jettyServer = ServersUtil.startServers();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        jettyServer.stop();
        System.setProperties(originalProperties);
    }

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new AtmosphereMessagingModule());
        Module modulesWithRuntime = Modules.override(modules).with(runtimeModule);
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);
        return application.getRuntime();
    }

    // Remove once we have support multicast for http / long polling
    @Ignore
    @Override
    public void testSimpleBroadcast() throws DiscoveryException, JoynrIllegalStateException, InterruptedException {
        // Noop
    }
}