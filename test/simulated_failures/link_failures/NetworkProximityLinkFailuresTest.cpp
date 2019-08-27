/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <boost/algorithm/string.hpp>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(network_proximity_link_failures, "Log category for NetworkProximityLinkFailuresTest");


class NetworkProximityLinkFailuresTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_NetworkProximityLinkFailures_Test();

protected:
    NetworkProximityLinkFailuresTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
                          "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"0us\"/>"
                          "       <link id=\"4\" bandwidth=\"3000GBps\" latency=\"0us\"/>"
                          "       <link id=\"5\" bandwidth=\"8000GBps\" latency=\"0us\"/>"
                          "       <link id=\"6\" bandwidth=\"2900GBps\" latency=\"0us\"/>"
                          "       <route src=\"StableHost\" dst=\"Host1\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "       <route src=\"StableHost\" dst=\"Host2\"> <link_ctn id=\"3\""
                          "/> </route>"
                          "       <route src=\"StableHost\" dst=\"Host3\"> <link_ctn id=\"4\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"5\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/**  FAILURE TEST                                                    **/
/**********************************************************************/

class NetworkProxLinkFailuresTestWMS : public wrench::WMS {

public:
    NetworkProxLinkFailuresTestWMS(NetworkProximityLinkFailuresTest *test,
                               std::set<std::shared_ptr<wrench::NetworkProximityService>> network_proximity_services,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {},
                        network_proximity_services, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    NetworkProximityLinkFailuresTest *test;

    int main() {
        
        for (int i=1; i <= 6; i++) {
            // Starting a link murderer!!
            auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, std::to_string(i),
                                                                                                   wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::LINK));
            murderer->simulation = this->simulation;
            murderer->start(murderer, true, false); // Daemonized, no auto-restart

            // Starting a link resurector!!
            auto resurector = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 300, std::to_string(i),
                                                                                                     wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::LINK));
            resurector->simulation = this->simulation;
            resurector->start(resurector, true, false); // Daemonized, no auto-restart
            
        }

        std::vector<std::string> hosts = {"Host1", "Host2", "Host3"};

        for (int trial=0; trial < 1000; trial++) {
            auto first_pair_to_compute_proximity = std::make_pair(hosts[trial%3], hosts[(37*trial+12)%3]);

            wrench::Simulation::sleep(100);
            try {
                (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                        first_pair_to_compute_proximity);
            } catch (wrench::WorkflowExecutionException &e) {
            }
        }

        return 0;
    }
};

TEST_F(NetworkProximityLinkFailuresTest, DISABLED_RandomLinkFailuress) {
    DO_TEST_WITH_FORK(do_NetworkProximityLinkFailures_Test);
}

void NetworkProximityLinkFailuresTest::do_NetworkProximityLinkFailures_Test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("one_task_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string stable_hostname = "StableHost";

    std::vector<std::string> hosts_in_network = {"Host1", "Host2", "Host3"};

    std::shared_ptr<wrench::NetworkProximityService> network_proximity_service;


    ASSERT_NO_THROW(network_proximity_service = simulation->add(new wrench::NetworkProximityService(stable_hostname, hosts_in_network, {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD,"10"},
                                                                                                                                        {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE,"0"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new NetworkProxLinkFailuresTestWMS(this,
                                           (std::set<std::shared_ptr<wrench::NetworkProximityService>>){network_proximity_service},
                                           stable_hostname)));

    wms->addWorkflow(this->workflow.get(), 0.0);



    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

