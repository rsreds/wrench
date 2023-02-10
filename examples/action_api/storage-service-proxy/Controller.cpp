
/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller to execute a multi-action job, where the actions
 ** read files from and XRootD deployment
 **/

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

#include <iostream>
#include <iomanip>
#include <utility>
#include "Controller.h"

/*
 * Helper function for pretty-printed output
 */
std::string padLong(long l) {
    return (l < 10 ? "0" + std::to_string(l) : std::to_string(l));
}

std::string padDouble(double l) {
    return (l < 10 ? "0" + std::to_string(l) : std::to_string(l));
}

std::string formatDate(double time) {
    if (time < 0) {
        return "Not Started";
    }
    long seconds = (long) time;
    double ms = time - (double) seconds;
    long minutes = seconds / 60;
    seconds %= 60;
    long hours = minutes / 60;
    minutes %= 60;
    long days = hours / 24;
    hours %= 24;

    return std::to_string(days) + "-" + padLong(hours) + ':' + padLong(minutes) + ':' + padDouble((double) seconds + ms);
}

WRENCH_LOG_CATEGORY(controller, "Log category for Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param bare_metal_compute_service: a bare-metal compute services available to run actions
     * @param proxy: The proxy storage service
     * @param remote: The remote storage service that is the default
     * @param target: The remote storage service that ISNT the default
     * @param hostname: the name of the host on which to start the Controller
     */
    Controller::Controller(const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                           const shared_ptr<StorageServiceProxy>& proxy,
                           const shared_ptr<StorageServiceProxy>& remote,
                           const shared_ptr<StorageServiceProxy>& target,
                           const std::string &hostname) : ExecutionController(hostname, "controller"),
                                                          bare_metal_compute_service(bare_metal_compute_service),
                                                          proxy(proxy),
                                                          remote(remote),
                                                          target(target){}

    /**
     * @brief main method of the Controller
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int Controller::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        WRENCH_INFO("Controller starting");

        ///* File Initilization */

        /* Add several 12-megabyte file to the simulation, which will
         * then be stored in various places
         */
        auto remoteFile = wrench::Simulation::addFile("remoteFile", 12 * MBYTE);
        auto targetFile = wrench::Simulation::addFile("targetFile", 12 * MBYTE);
        auto cachedFile = wrench::Simulation::addFile("cachedFile", 12 * MBYTE);

        /* create file on the default remote */
        remote->createFile(remoteFile);

        /* create file on the nondefault remote (target) */
        target->createFile(targetFile);

        /* create a file on default AND in the cache */
        remote->createFile(cachedFile);
        proxy->getCache->createFile(cachedFile);

        //proxy->createFile(cachedFile);//What this line should do is ambigous and not supported.  Use createFile directly on the underlying storage services


        ///* File Reading */

        /* read a file found on the default remote via the proxy */
        proxy->readFile(remoteFile);

        /* another way to do the same */
        StorageService::readFile(FileLocation::LOCATION(proxy,file));


        /* read a file found on target (non default remote) via the proxy */
        proxy->readFile(target,targetFile);

        /* another way to do the same */
        StorageService::readFile(ProxyLocation::LOCATION(target,proxy,targetFile));


        ///* File Writing */

        /* write to a file found on the default remote via the proxy */
        proxy->writeFile(remoteFile);

        /* another way to do the same */
        StorageService::writeFile(FileLocation::LOCATION(proxy,file));


        /* write to a file found on target (non default remote) via the proxy */
        proxy->writeFile(target,targetFile);

        /* another way to do the same */
        StorageService::writeFile(ProxyLocation::LOCATION(target,proxy,targetFile));

        ///* using jobs*/

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Create a compound job that just reads the file */
        WRENCH_INFO("Submitting a job that should successfully read the file from XRootD");
        auto job1 = job_manager->createCompoundJob("job1");
        /* read a file found on the default remote via the proxy */
        auto fileread1 = job1->addFileReadAction("fileread1", remoteFile, proxy);

        /* Submit the job that will succeed */
        job_manager->submitJob(job1, this->bare_metal_compute_service);

        /* Wait and process the next event, which will be a job success */
        this->waitForAndProcessNextEvent();

        /* read a file found on the non default target remote via the proxy */
        auto fileread2 = job1->addFileReadAction("fileread2", ProxyLocation::LOCATION(target, proxy,remoteFile));

        /* Wait and process the next event, which will be a job success */
        this->waitForAndProcessNextEvent();

        return 0;
    }

    /**
     * @brief Process a compound job completion event
     *
     * @param event: the event
     */
    void Controller::processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->job;
        WRENCH_INFO("Notified that compound job %s has successfully completed", job->getName().c_str());
    }

    /**
     * @brief Process a compound job completion event
     *
     * @param event: the event
     */
    void Controller::processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->job;
        auto action = *(job->getActions().begin());

        WRENCH_INFO("Notified that compound job %s has failed (%s)", job->getName().c_str(),
                    action->getFailureCause()->toString().c_str());
    }

}// namespace wrench
