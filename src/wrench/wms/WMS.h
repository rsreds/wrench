/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"
#include "wms/scheduler/Scheduler.h"
#include "wms/optimizations/dynamic/DynamicOptimization.h"
#include "wms/optimizations/static/StaticOptimization.h"
#include "workflow/Workflow.h"

namespace wrench {

    class Simulation; // forward ref

    /**
     * @brief Abstract implementation of a Workflow Management System
     */
    class WMS : public S4U_DaemonWithMailbox {

    public:
        void addStaticOptimization(std::unique_ptr<StaticOptimization>);

        void addDynamicOptimization(std::unique_ptr<DynamicOptimization>);


        /***********************/
        /** \cond DEVELOPER */
        /***********************/
    public:
        std::string getHostname();

    protected:

        friend class Simulation;

        WMS(Simulation *simulation,
            Workflow *workflow,
            std::unique_ptr<Scheduler> scheduler,
            std::string hostname,
            std::string suffix);

        void runDynamicOptimizations();

        void runStaticOptimizations();

        Simulation *simulation;
        Workflow *workflow;
        std::unique_ptr<Scheduler> scheduler;
        std::vector<std::unique_ptr<DynamicOptimization>> dynamic_optimizations;
        std::vector<std::unique_ptr<StaticOptimization>> static_optimizations;


        /***********************/
        /** \endcond           */
        /***********************/

    private:
        virtual int main() = 0;

        std::string hostname;
    };
};


#endif //WRENCH_WMS_H
