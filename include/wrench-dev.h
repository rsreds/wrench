/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WRENCH_DEV_H
#define WRENCH_WRENCH_DEV_H

#include "wrench.h"

// Exceptions
#include "exceptions/WorkflowExecutionException.h"

// Compute Services
#include "services/compute_services/ComputeService.h"
#include "services/compute_services/multicore_compute_service/MulticoreComputeService.h"

// Storage Services
#include "services/storage_services/StorageService.h"
#include "services/storage_services/simple_storage_service/SimpleStorageService.h"

// File Registry Service
#include "services/file_registry_service/FileRegistryService.h"

// Job Manager
#include "managers/job_manager/JobManager.h"

// Data Movement Manager
#include "managers/data_movement_manager/DataMovementManager.h"

// Logging
#include "logging/TerminalOutput.h"

// Workflow
#include "workflow/WorkflowTask.h"
#include "workflow/WorkflowFile.h"

// Workflow Job
#include "workflow_job/PilotJob.h"
#include "workflow_job/StandardJob.h"
#include "workflow_job/WorkflowJob.h"

#endif //WRENCH_WRENCH_DEV_H
