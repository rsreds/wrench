//
// Created by Dzung Do on 2020-08-03.
//

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/failure_causes/HostError.h"
#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_periodic_flush, "Log category for Periodic Flush");

namespace wrench {

    /**
     * Constructor
     *
     * @param memory: disk model used to simulate memory
     * @param dirty_ratio: dirty_ratio parameter as in the Linux kernel
     * @param interval: the interval that periodical flushing awakes in second
     * @param expired_time: the expired time of dirty data to be flushed in second
     * @param hostname: name of the host on which periodical flushing starts
     */
    MemoryManager::MemoryManager(s4u_Disk *memory, double dirty_ratio,
                                 int interval, int expired_time, std::string hostname) :
            Service(hostname, "periodic_flush_" + hostname, "periodic_flush_" + hostname),
            memory(memory), dirty_ratio(dirty_ratio), interval(interval), expired_time(expired_time) {

        free = S4U_Simulation::getHostMemoryCapacity(hostname);
        dirty = 0;
        cached = 0;
    }

    /**
     * Initialize and start the memory manager
     *
     * @param simulation
     * @param memory: disk model used to simulate memory
     * @param dirty_ratio: dirty_ratio parameter as in the Linux kernel
     * @param interval: the interval that periodical flushing awakes in milliseconds
     * @param expired_time: the expired time of dirty data to be flushed in milliseconds
     * @param hostname: name of the host on which periodical flushing starts
     * @return
     */
    std::shared_ptr<MemoryManager> MemoryManager::initAndStart(wrench::Simulation *simulation, s4u_Disk *memory,
                                                               double dirty_ratio, int interval, int expired_time,
                                                               std::string hostname) {

        std::shared_ptr<MemoryManager> memory_manager_ptr = std::shared_ptr<MemoryManager>(
                new MemoryManager(memory, dirty_ratio, interval, expired_time, hostname));

        memory_manager_ptr->simulation = simulation;

        try {
            memory_manager_ptr->start(memory_manager_ptr, true, false); // Daemonized, no auto-restart
        } catch (std::shared_ptr<HostError> &e) {
            throw;
        }
        return memory_manager_ptr;
    }

    int MemoryManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Periodic Flush starting with interval = %d , expired time = %d",
                this->interval, this->expired_time);

        while (this->getState() == State::UP) {
            WRENCH_INFO("Start periodical flushing on host %s", this->getHostname().c_str());

            double start_time = S4U_Simulation::getClock();
            pdflush();
            double end_time = S4U_Simulation::getClock();
            WRENCH_INFO("Periodical flushing completed in %lf sec",end_time - start_time);

            if (end_time - start_time < interval) {
                S4U_Simulation::sleep(interval + start_time - end_time);
            }
        }

        this->setStateToDown();
        return 0;
    }

    /**
     * @brief Immediately terminate periodical flushing
     */
    void MemoryManager::kill() {
        this->killActor();
    }

    s4u_Disk *MemoryManager::getMemory() const {
        return memory;
    }

    void MemoryManager::setMemory(s4u_Disk *memory) {
        this->memory = memory;
    }

    double MemoryManager::getDirtyRatio() const {
        return dirty_ratio;
    }

    void MemoryManager::setDirtyRatio(double dirtyRatio) {
        dirty_ratio = dirtyRatio;
    }

    double MemoryManager::getFree() const {
        return free;
    }

    void MemoryManager::setFree(double free_amt) {
        this->free = free_amt;
    }

    double MemoryManager::getCached() const {
        return cached;
    }

    double MemoryManager::getDirty() const {
        return dirty;
    }

    double MemoryManager::getEvictable() {
        double sum = 0;
        std::for_each(this->inactive_list.begin(), this->inactive_list.end(), [&] (Block *blk) {
            sum += blk->getSize();
        });
        return sum;
    }

    /**
     * Flush dirty data in a LRU list
     * @param list: the LRU list whose data will be flushed
     * @param amount: the amount requested to flush
     * @return flushed amount
     */
    double MemoryManager::flushLruList(std::vector<Block *> &list, double amount) {

        if (amount <= 0) return 0;
        double flushed = 0;

        for (auto blk : list) {

            if (blk->isDirty()) {
                if (flushed + blk->getSize() <= amount) {
                    // flush whole block
                    blk->setDirty(false);
                    flushed += blk->getSize();
                } else if (flushed < amount && amount < flushed + blk->getSize()) {

                    double blk_flushed = amount - flushed;
                    flushed = amount;
                    // split
                    blk->setSize(blk->getSize() - blk_flushed);

                    std::string fn = blk->getFilename();
                    inactive_list.push_back(new Block(fn, blk_flushed, blk->getLastAccess(), false));
                } else {
                    // done flushing
                    break;
                }
            }
        }

        dirty -= flushed;

        return flushed;
    }

    /**
     * Flush dirty data from cache
     * @param amount: request amount to be flushed
     * @return flushed amount
     */
    double MemoryManager::flush(std::string mountpoint, double amount) {

        double flushed_inactive = flushLruList(inactive_list, amount);

        double flushed_active = 0;
        if (flushed_inactive < amount) {
            flushed_active = flushLruList(active_list, amount - flushed_inactive);
        }

        s4u_Disk *disk = getDisk(mountpoint, this->hostname);
        disk->write(flushed_inactive + flushed_active);

        return flushed_inactive + flushed_active;
    }

    /**
     * Flush expired dirty data in a list.
     * Expired dirty data is the dirty data not accessed in a period longer than expired_time
     * @param list: the LRU to be flushed
     * @return flushed amount
     */
    double MemoryManager::flushExpiredData(std::vector<Block *> &list) {

        double flushed = 0;

        for (auto blk : list) {
            if (!blk->isDirty()) continue;
            if (S4U_Simulation::getClock() - blk->getLastAccess() >= expired_time) {
                blk->setDirty(false);
                flushed += blk->getSize();

                s4u_Disk *disk = getDisk(blk->getFilename(), hostname);
                disk->write(blk->getSize());
            }
        }

        dirty -= flushed;
        return flushed;
    }

    /**
     * Periodical flushing, which flushes expired dirty data in a list.
     * Expired dirty data is the dirty data not accessed in a period longer than expired_time
     * @return flushed amount
     */
    double MemoryManager::pdflush() {
        long flushed = 0;
        flushed += flushExpiredData(inactive_list);
        flushed += flushExpiredData(active_list);
        return flushed;
    }

    /**
     * Evicted clean data from cache.
     * @param amount: the requested amount of data to be flushed
     * @return flushed amount
     */
    double MemoryManager::evict(double amount) {

        if (amount <= 0) return 0;
        double evicted = 0;

        for (int i = 0; i < inactive_list.size(); i++) {

            Block *blk = inactive_list.at(i);

            if (!blk->isDirty()) continue;
            if (evicted + blk->getSize() <= amount) {
                evicted += blk->getSize();
                inactive_list.erase(inactive_list.begin() + i);
                i--;
            } else if (evicted < amount && evicted + blk->getSize() > amount) {
                blk->setSize(blk->getSize() - amount - evicted);
                // done eviction
                evicted = amount;
                break;
            } else {
                // done eviction
                break;
            }
        }

        cached -= evicted;
        free += evicted;

        return evicted;
    }

    /**
     * Read data from disk to cache.
     * @param filename
     * @param amount
     */
    simgrid::s4u::IoPtr MemoryManager::readToCache(std::string filename, std::string mount_point, double amount, bool async) {
        // Change stats
        free -= amount;
        cached += amount;
        // Push cached data to inactive list
        inactive_list.push_back(new Block(filename, amount, S4U_Simulation::getClock(), false));
        balanceAndSortCache();

        s4u_Disk *disk = getDisk(filename, this->hostname);
        return disk->read_async(amount);
    }

    /**
     * Simulate a read from cache, re-access and update cached file data
     * @param filename: name of the file read
     */
    simgrid::s4u::IoPtr MemoryManager::readFromCache(std::string filename, bool async) {

        double dirty_reaccessed = 0;
        double clean_reaccessed = 0;

        // Calculate dirty cached data
        for (int i = 0; i < inactive_list.size(); i++) {
            Block *blk = inactive_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {
                if (blk->isDirty()) {
                    dirty_reaccessed += blk->getSize();
                } else {
                    clean_reaccessed += blk->getSize();
                }
                // remove the existing old block
                inactive_list.erase(inactive_list.begin() + i);
                i--;
            }
        }

        // Calculate clean cached data
        for (int i = 0; i < active_list.size(); i++) {
            Block *blk = active_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {
                if (blk->isDirty()) {
                    dirty_reaccessed += blk->getSize();
                } else {
                    clean_reaccessed += blk->getSize();
                }

                // remove the existing old block
                active_list.erase(active_list.begin() + i);
                i--;
            }
        }

        // create new blocks and put in the active list
        if (clean_reaccessed > 0) {
            Block *new_blk = new Block(filename, clean_reaccessed, S4U_Simulation::getClock(), false);
            active_list.push_back(new_blk);
        }
        if (dirty_reaccessed > 0) {
            Block *new_blk = new Block(filename, dirty_reaccessed, S4U_Simulation::getClock(), true);
            active_list.push_back(new_blk);
        }

        balanceAndSortCache();

        return this->memory->read_async(clean_reaccessed + dirty_reaccessed);
    }

    /**
     * Simulate a file write to cache
     * @param filename: name of the file written
     * @param amount: amount of data written
     */
    void MemoryManager::writeToCache(std::string &filename, double amount) {

        Block *blk = new Block(filename, amount, S4U_Simulation::getClock(), true);
        inactive_list.push_back(blk);

        this->cached -= amount;
        this->free -= amount;
        this->dirty += amount;
    }

    bool compare_last_access(Block *blk1, Block *blk2) {
        return blk1->getLastAccess() < blk2->getLastAccess();
    }

    /**
     * Balance the LRU lists and sort these lists by last access of the blocks
     */
    void MemoryManager::balanceAndSortCache() {
        balanceLruLists();
        std::sort(active_list.begin(), active_list.end(), compare_last_access);
        std::sort(inactive_list.begin(), inactive_list.end(), compare_last_access);
    }

    /**
     * Balance the amount of data in LRU lists.
     * If the amount of data in the active list is more than doubled of the inactive list,
     * move blocks from the active list to the inactive list to make their sizes equal.
     */
    void MemoryManager::balanceLruLists() {

        double inactive_size = 0;
        std::for_each(inactive_list.begin(), inactive_list.end(), [&](Block *blk) {
            inactive_size += blk->getSize();
        });

        double active_size = 0;
        std::for_each(active_list.begin(), active_list.end(), [&](Block *blk) {
            active_size += blk->getSize();
        });

        if (active_size > 2 * inactive_size) {

            double to_move_amt = (active_size - inactive_size) / 2;
            double moved_amt = 0;

            for (int i = 0; i < active_list.size(); i++) {
                Block *blk = active_list.at(i);

                // move the whole block
                if (to_move_amt - (moved_amt + blk->getSize()) >= 0) {
                    inactive_list.push_back(blk);
                    active_list.erase(active_list.begin() + i);
                    i--;
                } else {
                    // split the block
                    std::string fn = blk->getFilename();
                    Block *new_blk = new Block(fn, to_move_amt - moved_amt, blk->getLastAccess(), blk->isDirty());
                    inactive_list.push_back(new_blk);

                    blk->setSize(blk->getSize() + moved_amt - to_move_amt);

                    // finish moving
                    break;
                }
            }

        }
    }

    /**
     * Get amount of cached data of a file in cache
     * @param filename: name of the file
     * @return the amount of cached data
     */
    double MemoryManager::getCachedData(std::string filename) {

        double amt = 0;

        std::for_each(inactive_list.begin(), inactive_list.end(), [&](Block *blk) {
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {
                amt += blk->getSize();
            }
        });

        std::for_each(active_list.begin(), active_list.end(), [&](Block *blk) {
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {
                amt += blk->getSize();
            }
        });

        return amt;
    }

    /**
     * Retrieve the disk where the file is stored.
     * @param mountpoint: mountpoint on the disk where the file is stored
     * @return
     */
    s4u_Disk *MemoryManager::getDisk(std::string mountpoint, std::string hostname) {

        mountpoint = FileLocation::sanitizePath(mountpoint);

        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (not host) {
            throw std::invalid_argument("MemoryManager::getDisk: unknown host " + hostname);
        }

        auto disk_list = simgrid::s4u::Host::by_name(hostname)->get_disks();
        for (auto disk : disk_list) {
            std::string disk_mountpoint =
                    FileLocation::sanitizePath(std::string(std::string(disk->get_property("mount"))));
            if (disk_mountpoint == mountpoint) {
                return disk;
            }
        }

        return nullptr;
    }

}