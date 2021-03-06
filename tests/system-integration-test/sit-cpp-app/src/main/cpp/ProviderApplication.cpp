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

#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <stdexcept>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"

#include "SystemIntegrationTestProvider.h"
#include "SitUtil.h"

#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

using namespace joynr;

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYSP", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    Logger logger("ProviderApplication");

    namespace po = boost::program_options;

    po::positional_options_description positionalCmdLineOptions;
    positionalCmdLineOptions.add("domain", 1);
    positionalCmdLineOptions.add("runForever", 1);

    std::string providerDomain;
    bool runForever = false;
    std::string pathToSettings;
    std::string sslCertFilename;
    std::string sslPrivateKeyFilename;
    std::string sslCaCertFilename;

    po::options_description cmdLineOptions("Available options");
    cmdLineOptions.add_options()(
            "domain,d", po::value(&providerDomain)->required(), "joynr domain to be used")(
            "runForever,r",
            po::value(&runForever)->default_value(false),
            "If not set the provider will terminate"
            "after answering the first request.")("pathtosettings,p",
                                                  po::value(&pathToSettings),
                                                  "Absolute path to a non-default setting file.")(
            "ssl-cert-pem",
            po::value(&sslCertFilename),
            "Absolute path to public certificate for this application.")(
            "ssl-privatekey-pem",
            po::value(&sslPrivateKeyFilename),
            "Absolute path to private key for this application.")(
            "ssl-ca-cert-pem",
            po::value(&sslCaCertFilename),
            "Absolute path to certificate of CA.")("help,h", "Print help message");

    try {
        po::variables_map variablesMap;
        po::store(po::command_line_parser(argc, argv)
                          .options(cmdLineOptions)
                          .positional(positionalCmdLineOptions)
                          .run(),
                  variablesMap);

        if (variablesMap.count("help")) {
            std::cout << cmdLineOptions << std::endl;
            return EXIT_SUCCESS;
        }

        po::notify(variablesMap);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << cmdLineOptions << std::endl;
        return EXIT_FAILURE;
    }

    if (pathToSettings.empty()) {
        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        pathToSettings = appDirectory + "/resources/systemintegrationtest-provider.settings";
    }

    std::shared_ptr<JoynrRuntime> runtime;
    try {
        runtime = joynr::sitUtil::createRuntime(
                pathToSettings, sslCertFilename, sslPrivateKeyFilename, sslCaCertFilename);
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_FATAL(logger, e.what());
        runtime.reset();
    }

    if (!runtime) {
        return EXIT_FAILURE;
    }

    try {
        joynr::Semaphore semaphore;

        JOYNR_LOG_INFO(logger, "Registering provider on domain {}", providerDomain);
        auto provider =
                std::make_shared<SystemIntegrationTestProvider>([&]() { semaphore.notify(); });

        joynr::types::ProviderQos providerQos;
        std::chrono::milliseconds millisSinceEpoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
        providerQos.setPriority(millisSinceEpoch.count());
        providerQos.setScope(joynr::types::ProviderScope::GLOBAL);

        runtime->registerProvider<test::SystemIntegrationTestProvider>(
                providerDomain, provider, providerQos);

        if (runForever) {
            while (true) {
                semaphore.wait();
            }
        } else {
            bool successful = semaphore.waitFor(std::chrono::milliseconds(30000));
            runtime->unregisterProvider<test::SystemIntegrationTestProvider>(
                    providerDomain, provider);

            return successful ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    } catch (exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_FATAL(logger, e.what());
        return EXIT_FAILURE;
    }
}
