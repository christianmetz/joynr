package io.joynr.integration.util;

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

import static io.joynr.integration.matchers.MonitoringServiceResponseMatchers.containsBounceProxy;
import io.joynr.integration.setup.SystemPropertyServletConfiguration;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.servlet.ServletUtil;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;

import org.eclipse.jetty.server.AbstractConnector;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Handler;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.ContextHandlerCollection;
import org.eclipse.jetty.server.nio.SelectChannelConnector;
import org.eclipse.jetty.server.ssl.SslSocketConnector;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.webapp.Configuration;
import org.eclipse.jetty.webapp.WebAppContext;
import org.eclipse.jetty.webapp.WebInfConfiguration;
import org.eclipse.jetty.webapp.WebXmlConfiguration;
import org.hamcrest.CoreMatchers;
import org.hamcrest.Matcher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.io.Resources;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

public class ServersUtil {
    public static final String BOUNCEPROXY_CONTEXT = "/bounceproxy";

    public static final String DISCOVERY_CONTEXT = "/discovery";

    public static final String BOUNCEPROXYCONTROLLER_CONTEXT = "/controller";

    private static final Logger logger = LoggerFactory.getLogger(ServersUtil.class);

    private static void setBounceProxyUrl() {
        String serverUrl = System.getProperties().getProperty("hostPath");
        String bounceProxyUrl = serverUrl + BOUNCEPROXY_CONTEXT + "/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
    }

    private static void setDirectoriesUrl() {
        String serverUrl = System.getProperties().getProperty("hostPath");
        String directoriesUrl = serverUrl + DISCOVERY_CONTEXT + "/channels/discoverydirectory_channelid/";

        System.setProperty(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL, directoriesUrl);
        System.setProperty(MessagingPropertyKeys.CHANNELURLDIRECTORYURL, directoriesUrl);
    }

    public static Server startServers() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp(), discoveryWebApp() });

        System.setProperty("log4j.configuration", Resources.getResource("log4j_backend.properties").toString());

        Server server = startServer(contexts);
        setBounceProxyUrl();
        setDirectoriesUrl();
        return server;
    }

    public static Server startSSLServers(SSLSettings settings) throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp(), discoveryWebApp() });

        Server server = startSSLServer(contexts, settings);
        setBounceProxyUrl();
        setDirectoriesUrl();
        return server;
    }

    public static Server startBounceproxy() throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyWebApp() });

        Server server = startServer(contexts);
        setBounceProxyUrl();
        return server;
    }

    public static Server startControlledBounceproxy(String bpId) throws Exception {

        final int port = ServletUtil.findFreePort();
        final String bpUrl = "http://localhost:" + port + "/bounceproxy/";

        System.setProperty("joynr.bounceproxy.id", bpId);
        System.setProperty("joynr.bounceproxy.controller.baseurl",
                           System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL));
        System.setProperty("joynr.bounceproxy.url4cc", bpUrl);
        System.setProperty("joynr.bounceproxy.url4bpc", bpUrl);

        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createControlledBounceproxyWebApp("", null) });
        Server server = startServer(contexts, port);

        System.clearProperty("joynr.bounceproxy.id");
        System.clearProperty("joynr.bounceproxy.controller.baseurl");
        System.clearProperty("joynr.bounceproxy.url4cc");
        System.clearProperty("joynr.bounceproxy.url4bpc");

        return server;
    }

    public static Server startBounceproxyController() throws Exception {
        return startBounceproxyController("bounceproxy-controller-nonclustered");
    }

    public static Server startClusteredBounceproxyController() throws Exception {
        return startBounceproxyController("bounceproxy-controller-clustered");
    }

    private static Server startBounceproxyController(String warFileName) throws Exception {
        ContextHandlerCollection contexts = new ContextHandlerCollection();
        contexts.setHandlers(new Handler[]{ createBounceproxyControllerWebApp(warFileName, "", null) });

        Server server = startServer(contexts);
        String serverUrl = System.getProperties().getProperty("hostPath");
        String bounceProxyUrl = serverUrl + "/controller/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
        return server;
    }

    private static Server startServer(ContextHandlerCollection contexts) throws IOException, Exception {
        final int port = ServletUtil.findFreePort();
        return startServer(contexts, port);
    }

    private static Server startSSLServer(ContextHandlerCollection contexts, SSLSettings settings) throws IOException,
                                                                                                 Exception {
        final int port = ServletUtil.findFreePort();
        return startSSLServer(contexts, settings, port);
    }

    private static Server startServer(ContextHandlerCollection contexts, int port) throws IOException, Exception {

        logger.info("PORT: http://localhost:{}", port);
        final Server jettyServer = new Server();
        AbstractConnector connector = new SelectChannelConnector();
        connector.setPort(port);
        connector.setAcceptors(1);
        jettyServer.setConnectors(new Connector[]{ connector });

        String serverUrl = "http://localhost:" + port;
        System.getProperties().setProperty("hostPath", serverUrl);

        jettyServer.setHandler(contexts);
        jettyServer.start();

        return jettyServer;
    }

    private static Server startSSLServer(ContextHandlerCollection contexts, SSLSettings settings, int port)
                                                                                                           throws IOException,
                                                                                                           Exception {

        logger.info("PORT: https://localhost:{}", port);
        final Server jettyServer = new Server();

        // Configure SSL
        final SslContextFactory contextFactory = new SslContextFactory();
        contextFactory.setKeyStorePath(settings.getKeyStorePath());
        contextFactory.setTrustStore(settings.getTrustStorePath());
        contextFactory.setKeyStorePassword(settings.getKeyStorePassword());
        contextFactory.setTrustStorePassword(settings.getKeyStorePassword());
        contextFactory.setNeedClientAuth(true);

        // Create and use an SSL connector
        SslSocketConnector connector = new SslSocketConnector(contextFactory);
        connector.setPort(port);
        connector.setAcceptors(1);
        jettyServer.setConnectors(new Connector[]{ connector });

        String serverUrl = "https://localhost:" + port;
        System.getProperties().setProperty("hostPath", serverUrl);

        jettyServer.setHandler(contexts);
        jettyServer.start();

        return jettyServer;
    }

    private static WebAppContext createBounceproxyWebApp() {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(BOUNCEPROXY_CONTEXT);
        bounceproxyWebapp.setWar("target/bounceproxy.war");
        return bounceproxyWebapp;
    }

    public static WebAppContext createControlledBounceproxyWebApp(String parentContext, Properties props) {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(createContextPath(parentContext, BOUNCEPROXY_CONTEXT));
        bounceproxyWebapp.setWar("target/controlled-bounceproxy.war");

        if (props != null) {
            bounceproxyWebapp.setConfigurations(new Configuration[]{ new WebInfConfiguration(),
                    new WebXmlConfiguration(), new SystemPropertyServletConfiguration(props) });
        }

        // Makes jetty load classes in the same order as JVM. Otherwise there's
        // a conflict loading loggers.
        bounceproxyWebapp.setParentLoaderPriority(true);

        return bounceproxyWebapp;
    }

    public static WebAppContext createBounceproxyControllerWebApp(String warFileName,
                                                                  String parentContext,
                                                                  Properties props) {
        WebAppContext bounceproxyWebapp = new WebAppContext();
        bounceproxyWebapp.setContextPath(createContextPath(parentContext, BOUNCEPROXYCONTROLLER_CONTEXT));
        bounceproxyWebapp.setWar("target/" + warFileName + ".war");

        if (props != null) {
            bounceproxyWebapp.setConfigurations(new Configuration[]{ new WebInfConfiguration(),
                    new WebXmlConfiguration(), new SystemPropertyServletConfiguration(props) });
        }
        // Makes jetty load classes in the same order as JVM. Otherwise there's
        // a conflict loading loggers.
        bounceproxyWebapp.setParentLoaderPriority(true);

        return bounceproxyWebapp;
    }

    /**
     * Creates a context path with slashes set at the right positions, i.e. a
     * leading slash, a single slash between each context and no slash at the
     * end.
     * 
     * @param contexts
     *            the contexts to add to the path. The contexts are added in the
     *            same order as given as parameters.
     * @return
     */
    private static String createContextPath(String... contexts) {

        StringBuffer resultContext = new StringBuffer();

        for (String context : contexts) {

            if (!(context == null || context.isEmpty())) {
                if (!context.startsWith("/")) {
                    resultContext.append("/");
                }

                if (context.endsWith("/")) {
                    resultContext.append(context.substring(0, context.length() - 2));
                } else {
                    resultContext.append(context);
                }
            }
        }

        return resultContext.toString();
    }

    private static WebAppContext discoveryWebApp() {
        WebAppContext discoveryWebapp = new WebAppContext();
        discoveryWebapp.setContextPath(DISCOVERY_CONTEXT);
        discoveryWebapp.setWar("target/discovery.war");
        return discoveryWebapp;
    }

    /**
     * Waits until all bounce proxies are registered with a single bounce proxy
     * controller or until the timeout is reached.
     * 
     * @param timeout_ms
     *            the timeout in milliseconds
     * @param wait_time_ms
     *            the time to wait between two requests to the bounceproxy
     *            controller to get a list of registered bounceproxies (in
     *            milliseconds)
     * @param bpControllerUrl
     *            the bounceproxy controller base url to retrieve the list of
     *            registered bounceproxies from
     * @param bpIds
     *            the list of bounce proxy IDs to check for their registration
     * @return <code>true</code> if all bounceproxies are registered at the
     *         bounceproxy controller and the timeout hasn't expired yet,
     *         <code>false</code> if timeout has expired before all
     *         bounceproxies were registered.
     * @throws InterruptedException
     *             if {@link Thread#sleep(long)} between two calls to the
     *             bounceproxy controller was interrupted
     */
    public static boolean waitForBounceProxyRegistration(long timeout_ms,
                                                         long wait_time_ms,
                                                         String bpControllerUrl,
                                                         String... bpIds) throws InterruptedException {

        logger.debug("Wait for registration of bounceproxies at controller URL {} for {} ms",
                     bpControllerUrl,
                     timeout_ms);

        long start = System.currentTimeMillis();

        while (!areBounceProxiesRegisteredWithController(bpControllerUrl, bpIds)) {

            if (System.currentTimeMillis() - start > timeout_ms) {
                return false;
            }

            Thread.sleep(1000l);
        }
        return true;
    }

    private static boolean areBounceProxiesRegisteredWithController(String bpControllerUrl, String... bpIds) {

        List<Matcher<? super JsonPath>> containsBounceProxyMatcher = new LinkedList<Matcher<? super JsonPath>>();

        for (String bpId : bpIds) {
            Matcher<? super JsonPath> matcher = CoreMatchers.anyOf(containsBounceProxy(bpId, "ALIVE"),
                                                                   containsBounceProxy(bpId, "ACTIVE"));
            containsBounceProxyMatcher.add(matcher);
        }

        Matcher<JsonPath> matcher = CoreMatchers.allOf(containsBounceProxyMatcher);

        Response bounceProxiesResponse = getBounceProxies(bpControllerUrl);
        return matcher.matches(bounceProxiesResponse.jsonPath());
    }

    private static Response getBounceProxies(String bpControllerUrl) {

        String previousBaseUri = RestAssured.baseURI;
        RestAssured.baseURI = bpControllerUrl;
        Response bounceProxiesResponse = RestAssured.given().get("bounceproxies");
        RestAssured.baseURI = previousBaseUri;
        return bounceProxiesResponse;
    }

}
