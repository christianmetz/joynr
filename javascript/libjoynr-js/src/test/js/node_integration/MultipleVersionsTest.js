/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
const RequireUtil = require("./RequireUtil");

let IntegrationUtils;
let MultipleVersionsInterfaceProviderNameVersion1;
let MultipleVersionsInterfaceProviderNameVersion2;
let MultipleVersionsInterfaceProviderPackageVersion1;
let MultipleVersionsInterfaceProviderPackageVersion2;
let MultipleVersionsInterfaceProviderUnversioned;
let MultipleVersionsInterfaceProxyNameVersion1;
let MultipleVersionsInterfaceProxyNameVersion2;
let MultipleVersionsInterfaceProxyPackageVersion2;
let ProviderImplementation;
let provisioning;
let joynr;

const domain = "MultipleVersionsTestDomain";
const requirePaths = new Map([
    ["IntegrationUtils", require.resolve("./IntegrationUtils")],
    [
        "MultipleVersionsInterfaceProviderNameVersion1",
        require.resolve("../../generated/joynr/tests/MultipleVersionsInterface1Provider")
    ],
    [
        "MultipleVersionsInterfaceProviderNameVersion2",
        require.resolve("../../generated/joynr/tests/MultipleVersionsInterface2Provider")
    ],
    [
        "MultipleVersionsInterfaceProviderPackageVersion1",
        require.resolve("../../generated/joynr/tests/v1/MultipleVersionsInterfaceProvider")
    ],
    [
        "MultipleVersionsInterfaceProviderPackageVersion2",
        require.resolve("../../generated/joynr/tests/v2/MultipleVersionsInterfaceProvider")
    ],
    [
        "MultipleVersionsInterfaceProviderUnversioned",
        require.resolve("../../generated/joynr/tests/MultipleVersionsInterfaceProvider")
    ],
    [
        "MultipleVersionsInterfaceProxyNameVersion1",
        require.resolve("../../generated/joynr/tests/MultipleVersionsInterface1Proxy")
    ],
    [
        "MultipleVersionsInterfaceProxyNameVersion2",
        require.resolve("../../generated/joynr/tests/MultipleVersionsInterface2Proxy")
    ],
    [
        "MultipleVersionsInterfaceProxyPackageVersion2",
        require.resolve("../../generated/joynr/tests/v2/MultipleVersionsInterfaceProxy")
    ],
    ["ProviderImplementation", require.resolve("./MultipleVersionsInterfaceProviderImplementation")],
    ["provisioning", require.resolve("../../resources/joynr/provisioning/provisioning_cc.js")],
    ["joynr", require.resolve("joynr")]
]);

describe("libjoynr-js.integration.MultipleVersionsTest", () => {
    beforeEach(async () => {
        eval(RequireUtil.safeRequire(requirePaths));

        joynr.loaded = false;

        joynr.selectRuntime("inprocess");
        await joynr.load(provisioning);
    });

    afterEach(async () => {
        await joynr.shutdown();

        RequireUtil.deleteFromCache(requirePaths);
    });

    async function buildProvider(providerType) {
        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: 5,
            scope: joynr.types.ProviderScope.LOCAL,
            supportsOnChangeSubscriptions: false
        });
        const multipleVersionsInterfaceProvider = joynr.providerBuilder.build(
            providerType,
            new ProviderImplementation()
        );

        await joynr.registration.registerProvider(domain, multipleVersionsInterfaceProvider, providerQos);

        return multipleVersionsInterfaceProvider;
    }

    async function buildProxy(proxyType) {
        const discoveryQos = new joynr.proxy.DiscoveryQos({
            discoveryTimeoutMs: 1000
        });
        return await joynr.proxyBuilder.build(proxyType, {
            domain,
            discoveryQos
        });
    }

    // set same attribute of different providers and check if right result is obtained.
    async function setUInt8AttributesAndCheck(proxy1, proxy2) {
        // set attributes
        await Promise.all([proxy1.uInt8Attribute1.set({ value: 50 }), proxy2.uInt8Attribute1.set({ value: 100 })]);

        // retreive attributes
        const [value1, value2] = await Promise.all([proxy1.uInt8Attribute1.get(), proxy2.uInt8Attribute1.get()]);

        // check if received values equal set values.
        expect(value1).toEqual(50);
        expect(value2).toEqual(100);
    }

    /**
     * Two providers of different version can not be registered in a single runtime.
     * (They are both registered with the same participant id and thus seen as one.)
     * @Todo: Get this test to pass.
     */
    it("2 proxies and 2 providers of different versions in same runtime (name version vs. name version)", async () => {
        // build and register providers
        const [provider1, provider2] = await Promise.all([
            buildProvider(MultipleVersionsInterfaceProviderNameVersion1),
            buildProvider(MultipleVersionsInterfaceProviderNameVersion2)
        ]);

        // build fitting proxies
        const [proxy1, proxy2] = await Promise.all([
            buildProxy(MultipleVersionsInterfaceProxyNameVersion1),
            buildProxy(MultipleVersionsInterfaceProxyNameVersion2)
        ]);

        // set primitive type attributes and check if values can be retrieved properly by proxies
        // without mutual interference
        await setUInt8AttributesAndCheck(proxy1, proxy2);

        // unregister providers
        await Promise.all([
            joynr.registration.unregisterProvider(domain, provider1),
            joynr.registration.unregisterProvider(domain, provider2)
        ]);
    });

    /**
     * Two providers of different version can not be registered in a single runtime.
     * (They are both registered with the same participant id and thus seen as one.)
     * @Todo: Get this test to pass.
     */
    it("2 proxies and 2 providers of different versions in same runtime (name version vs. package version)", async () => {
        // build and register providers
        const [provider1, provider2] = await Promise.all([
            buildProvider(MultipleVersionsInterfaceProviderPackageVersion1),
            buildProvider(MultipleVersionsInterfaceProviderPackageVersion2)
        ]);

        // build fitting proxies
        const [proxy1, proxy2] = await Promise.all([
            buildProxy(MultipleVersionsInterfaceProxyNameVersion1),
            buildProxy(MultipleVersionsInterfaceProxyNameVersion2)
        ]);

        // set primitive type attributes and check if values can be retrieved properly by proxies
        // without mutual interference
        await setUInt8AttributesAndCheck(proxy1, proxy2);

        // unregister providers
        await Promise.all([
            joynr.registration.unregisterProvider(domain, provider1),
            joynr.registration.unregisterProvider(domain, provider2)
        ]);
    });

    it("2 proxies of different versioning types vs. unversioned provider", async () => {
        // build and register provider
        const multipleVersionsInterfaceProvider1 = await buildProvider(MultipleVersionsInterfaceProviderUnversioned);
        // build fitting proxies
        const [proxy1, proxy2] = await Promise.all([
            buildProxy(MultipleVersionsInterfaceProxyNameVersion2),
            buildProxy(MultipleVersionsInterfaceProxyPackageVersion2)
        ]);

        // check if attributes can be set and retrieved correctly by both proxies
        await Promise.all([proxy1.uInt8Attribute2.set({ value: 100 }), proxy2.uInt8Attribute1.set({ value: 50 })]);

        const [value1, value2] = await Promise.all([proxy1.uInt8Attribute1.get(), proxy2.uInt8Attribute2.get()]);

        expect(value1).toEqual(50);
        expect(value2).toEqual(100);

        await joynr.registration.unregisterProvider(domain, multipleVersionsInterfaceProvider1);
    });

    it("2 proxies connecting to 2 providers in different ChildProcesses subscribing to the same type (non primitive)", async () => {
        // register 2 providers in 2 child processes
        const [childId1, childId2] = await Promise.all([
            IntegrationUtils.initializeChildProcess("TestMultipleVersionsInterfaceProcess", "", domain, {
                versioning: "nameVersion1"
            }),
            IntegrationUtils.initializeChildProcess("TestMultipleVersionsInterfaceProcess", "", domain, {
                versioning: "nameVersion2"
            })
        ]);

        // build fitting proxies
        const [proxy1, proxy2] = await Promise.all([
            buildProxy(MultipleVersionsInterfaceProxyNameVersion1),
            buildProxy(MultipleVersionsInterfaceProxyNameVersion2)
        ]);

        // set attributes and check if values can be retrieved properly by proxies without mutual interference
        await setUInt8AttributesAndCheck(proxy1, proxy2);

        // unregister providers and kill child processes
        await Promise.all([
            IntegrationUtils.shutdownChildProcess(childId2),
            IntegrationUtils.shutdownChildProcess(childId1)
        ]);
    });
});
