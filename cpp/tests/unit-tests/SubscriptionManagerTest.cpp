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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/SubscriptionManager.h"
#include "joynr/ISubscriptionCallback.h"
#include "tests/utils/MockObjects.h"
#include "utils/TestQString.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/SingleThreadedDelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/ThreadUtil.h"
#include "joynr/TimeUtils.h"
#include "joynr/joynrlogging.h"
#include "joynr/Directory.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/Util.h"
#include <chrono>
#include <stdint.h>

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;

MATCHER_P(publicationMissedException, subscriptionId, "") {
    if (arg.getTypeName() == joynr::exceptions::PublicationMissedException::TYPE_NAME) {
        joynr::exceptions::PublicationMissedException *errorArg = dynamic_cast<joynr::exceptions::PublicationMissedException*>(arg.clone());
        bool success = errorArg->getSubscriptionId() == subscriptionId && errorArg->getMessage() == subscriptionId;
        delete errorArg;
        return success;
    }
    return false;
}

using namespace joynr;

TEST(SubscriptionManagerTest, registerSubscription_subscriptionRequestIsCorrect) {
    SubscriptionManager subscriptionManager;
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
                new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    OnChangeSubscriptionQos qos{};
    int64_t now = TimeUtils::getCurrentMillisSinceEpoch();
    qos.setExpiryDate(now + 10000);
    Variant qosVariant = Variant::make<OnChangeSubscriptionQos>(qos);
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qosVariant,
                subscriptionRequest);

    EXPECT_EQ("methodName", subscriptionRequest.getSubscribeToName());
    EXPECT_EQ(qos, subscriptionRequest.getQos().get<OnChangeSubscriptionQos>());
}

TEST(SubscriptionManagerTest, registerSubscription_missedPublicationRunnableWorks) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtLeast(4));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1100, 100, 200));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    ThreadUtil::sleepForMillis(1200);
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_missedPublicationRunnableWorks) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtMost(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1100, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
    ThreadUtil::sleepForMillis(300);

    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener2(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    EXPECT_CALL(*mockGpsSubscriptionListener2,
                onError(_))
            .Times(0);
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback2(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener2));
    Variant qos2 = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(700, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback2,
                qos2,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    ThreadUtil::sleepForMillis(900);
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithEnlargedExpiryDate) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtLeast(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(300, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1000, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    ThreadUtil::sleepForMillis(1000);
}

TEST(SubscriptionManagerTest, registerSubscriptionWithSameSubscriptionId_correctDealingWithReducedExpiryDate) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(AtMost(6));
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation> > gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    SubscriptionManager subscriptionManager;
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(1000, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(300, 100, 100));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );

    // now, no new publicationMissed callbacks are expected for the first subscriptionRequest
    ThreadUtil::sleepForMillis(1000);
}

TEST(SubscriptionManagerTest, registerSubscription_withoutExpiryDate) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation>> gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler();
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(_,_))
            .Times(0);
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(-1, 100));
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
}

DelayedScheduler::RunnableHandle internalRunnableHandle = 1;

DelayedScheduler::RunnableHandle runnableHandle()
{
    return internalRunnableHandle++;
}

TEST(SubscriptionManagerTest, registerSubscription_withExpiryDate) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation>> gpslocationCallback(
            new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    MockDelayedScheduler* mockDelayedScheduler = new MockDelayedScheduler();
    EXPECT_CALL(*mockDelayedScheduler,
                schedule(A<Runnable*>(),_))
            .Times(1).WillRepeatedly(::testing::Return(runnableHandle()));
    SubscriptionManager subscriptionManager(mockDelayedScheduler);
    Variant qos = Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(1000, 100));
    SubscriptionRequest subscriptionRequest;
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest
    );
}

TEST(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsToStoppingMissedPublicationRunnables) {
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionRequest subscriptionRequest;
    EXPECT_CALL(*mockGpsSubscriptionListener,
                onError(publicationMissedException(subscriptionRequest.getSubscriptionId())))
            .Times(Between(2,3));
    SubscriptionManager subscriptionManager;
    std::shared_ptr<SubscriptionCallback<types::Localisation::QtGpsLocation>> gpslocationCallback(
                new SubscriptionCallback<types::Localisation::QtGpsLocation>(mockGpsSubscriptionListener));
    Variant qos = Variant::make<PeriodicSubscriptionQos>(PeriodicSubscriptionQos(
                2000, // validity
                100,  // period
                400   // alert after interval
    ));
    subscriptionManager.registerSubscription(
                "methodName",
                gpslocationCallback,
                qos,
                subscriptionRequest);
     ThreadUtil::sleepForMillis(900);
     subscriptionManager.unregisterSubscription(QString::fromStdString(subscriptionRequest.getSubscriptionId()));
     ThreadUtil::sleepForMillis(1100);
}

TEST(SubscriptionManagerTest, unregisterSubscription_unregisterLeadsOnNonExistantSubscription) {
    std::shared_ptr<ISubscriptionListener<types::Localisation::QtGpsLocation> > mockGpsSubscriptionListener(
            new MockSubscriptionListenerOneType<types::Localisation::QtGpsLocation>()
    );
    SubscriptionManager subscriptionManager;
    subscriptionManager.unregisterSubscription("superId");
}

class TestRunnable : public Runnable {
public:
    virtual ~TestRunnable() {

    }
    TestRunnable()
        : joynr::Runnable(true)
    {

    }
    void shutdown() {
        LOG_TRACE(logger, "shutdown called...");
    }
    void run() {
        LOG_TRACE(logger, "run: entering...");
        joynr::ThreadUtil::sleepForMillis(200);
        LOG_TRACE(logger, "run: leaving...");
    }
private:
    static joynr_logging::Logger* logger;
};
joynr_logging::Logger* TestRunnable::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "TestRunnable");

TEST(SingleThreadedDelayedSchedulerTest, schedule_deletingRunnablesCorrectly) {
    SingleThreadedDelayedScheduler scheduler("SingleThread");
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, 1);
    joynr::ThreadUtil::sleepForMillis(100);
    scheduler.shutdown();
}

TEST(ThreadPoolDelayedSchedulerTest, schedule_deletingRunnablesCorrectly) {
    ThreadPoolDelayedScheduler scheduler(3, "ThreadPool", 0);
    TestRunnable* runnable = new TestRunnable();
    scheduler.schedule(runnable, 1);
    joynr::ThreadUtil::sleepForMillis(100);
    scheduler.shutdown();
}
