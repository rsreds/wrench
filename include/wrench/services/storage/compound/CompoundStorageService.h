/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPOUNDSTORAGESERVICE_H
#define WRENCH_COMPOUNDSTORAGESERVICE_H

#include "wrench/services/storage/StorageService.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/memory/MemoryManager.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "wrench/services/storage/compound/CompoundStorageServiceProperty.h"
#include "wrench/services/storage/compound/CompoundStorageServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Specification of a callback
     */
    using StorageSelectionStrategyCallback = std::function<std::shared_ptr<FileLocation>(
            const std::shared_ptr<DataFile> &,
            const std::set<std::shared_ptr<StorageService>> &,
            const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &)>;

    /**
     * @brief An abstract storage service which holds a collection of concrete storage services (eg. 
     *        SimpleStorageServices). It does not provide direct access to any storage resource.
     *        It is meant to be used as a way to postpone the selection of a storage service for a file 
     *        action (read, write, copy, etc) until a later time in the simulation, rather than during
     *        job definition. A typical use for the CompoundStorageService is to select a definitive
     *        SimpleStorageService for each action of a job during its scheduling in a BatchScheduler class.
     *        This service is able to receive and handle the same messages as any standard storage service 
     *        (File Read/Write/Delete/Copy/Lookup requests), but will always answer that it is unable to 
     *        process any of these requests, which should be addressed directly to the correct underlying 
     *        storage service. (A possible future patch could give it the ability to automatically forward 
     *        said request to one of the underlying storage services).
     */
    class CompoundStorageService : public StorageService {
    public:
        CompoundStorageService(const std::string &hostname,
                               std::set<std::shared_ptr<StorageService>> storage_services,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        CompoundStorageService(const std::string &hostname,
                               std::set<std::shared_ptr<StorageService>> storage_services,
                               StorageSelectionStrategyCallback storage_selection,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) override;

        double getLoad() override;

        // Overload StorageService's implementation.
        std::map<std::string, double> getTotalSpace();

        // Overload StorageService's implementation.
        std::map<std::string, double> getFreeSpace();

        // Overload StorageService's implementation.
        void setScratch();

        /**
         * @brief Method to return the collection of known StorageServices
         */
        std::set<std::shared_ptr<StorageService>> &getAllServices();

        std::shared_ptr<FileLocation> lookupFileLocation(const std::shared_ptr<DataFile> &file);

        std::shared_ptr<FileLocation> lookupFileLocation(const std::shared_ptr<FileLocation> &location);

    protected:
        CompoundStorageService(const std::string &hostname,
                               std::set<std::shared_ptr<StorageService>> storage_services,
                               StorageSelectionStrategyCallback storage_selection,
                               bool storage_selection_user_provided,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
                               const std::string &suffix);

        /** @brief Default property values **/
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {CompoundStorageServiceProperty::STORAGE_SELECTION_METHOD, "external"},
                {CompoundStorageServiceProperty::CACHING_BEHAVIOR, "NONE"},
        };

        /** @brief Default message payload values 
         *         Some values are set to zero because in the current implementation, it is expected
         *         that the CompoundStorageService will always immediately refuse / reject such
         *         requests, with minimum cost to the user.
         */
        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {CompoundStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {CompoundStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {CompoundStorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, 0},
        };

        static unsigned long getNewUniqueNumber();

        bool processStopDaemonRequest(simgrid::s4u::Mailbox *ack_mailbox);

        bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path) override;

    private:
        friend class Simulation;

        int main() override;

        std::shared_ptr<FileLocation> lookupOrDesignateStorageService(const std::shared_ptr<DataFile> concrete_file_location);

        std::shared_ptr<FileLocation> lookupOrDesignateStorageService(const std::shared_ptr<FileLocation> location);

        bool processNextMessage(SimulationMessage *message);

        bool processFileDeleteRequest(StorageServiceFileDeleteRequestMessage *msg);

        bool processFileLookupRequest(StorageServiceFileLookupRequestMessage *msg);

        bool processFileCopyRequest(StorageServiceFileCopyRequestMessage *msg);

        bool processFileWriteRequest(StorageServiceFileWriteRequestMessage *msg);

        bool processFileReadRequest(StorageServiceFileReadRequestMessage *msg);

        std::set<std::shared_ptr<StorageService>> storage_services = {};

        std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_location_mapping = {};

        StorageSelectionStrategyCallback storage_selection;

        bool isStorageSelectionUserProvided;
    };

};// namespace wrench

#endif//WRENCH_COMPOUNDSTORAGESERVICE_H
