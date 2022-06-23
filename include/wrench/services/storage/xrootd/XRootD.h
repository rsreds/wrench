/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_XROOTD_H
#define WRENCH_XROOTD_H
#include "wrench/services/Service.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include "wrench/data_file/DataFile.h"
#include <set>
#include "wrench/simulation/Simulation.h"
namespace wrench {
    /**
     * @brief A Meta manager for an XRootD data Federation.  This tracks all nodes and files within the system.
     */
    namespace XRootD{
        //class StorageServer;
        //class Supervisor;
        class Node;

        class XRootD{
        public:

            /**
             * @brief Create an XRootD manager
             * @param simulation: the simulation that all nodes run in.  Nodes are automatically added to this simulation as created.
             */
            XRootD(std::shared_ptr<Simulation>  simulation):simulation(simulation){}
            std::shared_ptr<Node> createStorageServer(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);
            std::shared_ptr<Node> createSupervisor(const std::string& hostname);
            std::shared_ptr<Node> createStorageSupervisor(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE property_list, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);
            /***********************/
            /** \cond DEVELOPER    */
            /***********************/
            /** @brief The max number of hops a search message can take.  Used to prevent infinite loops in a poorly constructed XRootD tree. */
            int defaultTimeToLive=1024;//how long trivial search message can wander for;
            void createFile(const std::shared_ptr<DataFile> &file, const std::shared_ptr<Node> &location);
            unsigned int size();
        private:
            /***********************/
            /** \cond INTERNAL     */
            /***********************/
            friend Node;
            std::vector<std::shared_ptr<Node>> getFileNodes(std::shared_ptr<DataFile> file);
            std::shared_ptr<Node> createNode(const std::string& hostname);
            /** @brief All nodes that are connected to this XRootD data Federation */
            std::vector<std::shared_ptr<Node>> nodes;
            /** @brief All nodes in the XRootD Federation that have and internal file server */
            std::vector<std::shared_ptr<Node>> dataservers;
            /** @brief All nodes in the XRootD Federation that have child nodes */
            std::vector<std::shared_ptr<Node>> supervisors;
            /** @brief All files within the data federation regardless of which server */
            std::unordered_map<std::shared_ptr<DataFile> ,std::vector<std::shared_ptr<Node>>> files;
            /** @brief The simulation that this XRootD federation is connected too */
            std::shared_ptr<Simulation>  simulation;
            /***********************/
            /** \endcond           */
            /***********************/
        };
    }
}
#endif //WRENCH_XROOTD_H
