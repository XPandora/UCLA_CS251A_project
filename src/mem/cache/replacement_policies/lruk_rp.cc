/**
 * Copyright (c) 2018-2020 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/cache/replacement_policies/lruk_rp.hh"

#include <cassert>
#include <memory>

#include "params/LRUKRP.hh"
#include "sim/cur_tick.hh"

namespace gem5
{

    GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
    namespace replacement_policy
    {

        LRUK::LRUK(const Params &p)
            : Base(p)
        {
        }

        void
        LRUK::replaceLRUHistory(HistoryData history_data, std::vector<HistoryData> &history)
        {
            Tick min_Tick = history[0].lastTouchTick;
            int pos = 0;
            for (int i = 1; i < history.size(); i++)
            {
                if (history[i].lastTouchTick < min_Tick)
                {
                    pos = i;
                    min_Tick = history[i].lastTouchTick;
                }
            }

            history[pos] = history_data;
            history_tables[current_index] = history;
        }

        int
        LRUK::findHistory(uint64_t tag, const std::vector<HistoryData> &history)
        {
            for (int i = 0; i < history.size(); i++)
            {
                if (history[i].tag == tag)
                    return i;
            }
            return -1;
        }

        void addHistory(uint64_t tag, std::vector<HistoryData> &history)
        {
            HistoryData history_data(tag);
            history_data.lastTouchTick = curTick();
            history_data.refCount = 1;
            if (history.size() < max_history)
                history.push_back(history_data);
            else
                replaceLRUHistory(history_data, history);
        }

        void
        LRUK::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
        {
            // Reset last touch timestamp
            std::static_pointer_cast<LRUReplData>(
                replacement_data)
                ->lastTouchTick = Tick(0);
        }

        void
        LRUK::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // Update last touch timestamp
            std::vector<HistoryData> history = history_tables[current_index];
            int pos = findHistory(current_tag, history);
            if (pos >= 0)
            {
                history[pos].lastTouchTick = curTick();
                history[pos].refCount++;

                if (history[pos].refCount >= k)
                {
                    history.erase(history.begin() + pos);
                    std::static_pointer_cast<LRUReplData>(replacement_data)->lastTouchTick = curTick();
                }
            }
            else
            {
                addHistory(current_tag, history);
            }
            history_tables[current_index] = history;
        }

        void
        LRUK::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // Set last touch timestamp
            std::static_pointer_cast<LRUReplData>(
                replacement_data)
                ->lastTouchTick = curTick();
        }

        ReplaceableEntry *
        LRUK::getVictim(const ReplacementCandidates &candidates) const
        {
            // There must be at least one replacement candidate
            assert(candidates.size() > 0);
            ReplaceableEntry *victim = NULL;
            std::vector<HistoryData> history = history_tables[current_index];
            int pos = findHistory(current_tag, history);
            if (pos >= 0)
            {
                history[pos].lastTouchTick = curTick();
                history[pos].refCount++;

                if (history[pos].refCount >= k)
                {
                    history.erase(history.begin() + pos);
                    // Visit all candidates to find victim
                    victim = candidates[0];
                    for (const auto &candidate : candidates)
                    {
                        // Update victim entry if necessary
                        if (std::static_pointer_cast<LRUReplData>(
                                candidate->replacementData)
                                ->lastTouchTick <
                            std::static_pointer_cast<LRUReplData>(
                                victim->replacementData)
                                ->lastTouchTick)
                        {
                            victim = candidate;
                        }
                    }
                }
            }
            else
            {
                addHistory(current_tag, history);
            }

            history_tables[current_index] = history;
            return victim;
        }

        std::shared_ptr<ReplacementData>
        LRUK::instantiateEntry()
        {
            this->block_num++;
            if (current_index >= history_tables.size())
            {
                std::vector<HistoryData> history;
                history_table.push_back(history);
            }

            this->c = std::max(c, this->block_num / history_tables.size());
            this->max_history = 4 * this->c;
            return std::shared_ptr<ReplacementData>(new LRUReplData());
        }

    } // namespace replacement_policy
} // namespace gem5
