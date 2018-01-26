//
// Created by suraj on 9/16/17.
//

#ifndef WRENCH_BATCHSERVICEMESSAGE_H
#define WRENCH_BATCHSERVICEMESSAGE_H


#include "wrench/services/compute/ComputeServiceMessage.h"
#include "BatchJob.h"

namespace wrench{
    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level BatchServiceMessage class
     */
    class BatchServiceMessage : public ComputeServiceMessage {
    protected:
        BatchServiceMessage(std::string name, double payload);
    };

    /**
     * @brief BatchServiceJobRequestMessage class
     */
    class BatchServiceJobRequestMessage : public BatchServiceMessage {
    public:
        BatchServiceJobRequestMessage(std::string answer_mailbox, std::unique_ptr<BatchJob> job , double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch job */
        std::unique_ptr<BatchJob> job;
    };

    /**
     * @brief AlarmJobTimeOutMessage class
     */
    class AlarmJobTimeOutMessage : public ServiceMessage {
    public:
        AlarmJobTimeOutMessage(WorkflowJob* job,double payload);
        WorkflowJob* job;
    };


    /**
     * @brief BatchSimulationBeginsToSchedulerMessage class
     */
    class BatchSimulationBeginsToSchedulerMessage : public BatchServiceMessage {
    public:
        BatchSimulationBeginsToSchedulerMessage(std::string answer_mailbox, std::string job_args_to_scheduler, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief JSON data arguments to the scheduler */
        std::string job_args_to_scheduler;
    };

    /**
     * @brief BatchSchedReadyMessage class
     */
    class BatchSchedReadyMessage : public BatchServiceMessage {
    public:
        BatchSchedReadyMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
    };


    /**
     * @brief BatchExecuteJobFromBatschedMessage class
     */
    class BatchExecuteJobFromBatSchedMessage : public BatchServiceMessage {
    public:
        BatchExecuteJobFromBatSchedMessage(std::string answer_mailbox, std::string batsched_decision_reply, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;

        /** @brief The decisions reply by batsched to the batchservice */
        std::string batsched_decision_reply;
    };


    /**
     * @brief BatchJobSubmissionToSchedulerMessage class
     */
    class BatchJobSubmissionToSchedulerMessage : public BatchServiceMessage {
    public:
        BatchJobSubmissionToSchedulerMessage(std::string answer_mailbox, WorkflowJob* job, std::string job_args_to_scheduler, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch job */
        WorkflowJob *job;
        /** @brief JSON data arguments to the scheduler */
        std::string job_args_to_scheduler;
    };

    /**
     * @brief BatchJobReplyFromSchedulerMessage class
     */
    class BatchJobReplyFromSchedulerMessage : public BatchServiceMessage {
    public:
        BatchJobReplyFromSchedulerMessage(std::string reply, double payload);

        /** @brief The message replied by the scheduler */
        std::string reply;
    };



}


#endif //WRENCH_BATCHSERVICEMESSAGE_H
