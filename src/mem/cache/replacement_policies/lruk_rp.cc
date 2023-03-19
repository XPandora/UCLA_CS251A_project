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
        LRUK::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
        {
            // Reset last touch timestamp
            std::static_pointer_cast<LRUKReplData>(
                replacement_data)
                ->lastTouchTick = Tick(0);

            std::static_pointer_cast<LRUKReplData>(
                replacement_data)
                ->history.assign(k, 0);
        }

        void
        LRUK::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // Update last touch timestamp
            Tick cur_tick = curTick();
            if (cur_tick > std::static_pointer_cast<LRUKReplData>(replacement_data)->lastTouchTick + CPR)
            {
                std::vector<Tick> history = std::static_pointer_cast<LRUKReplData>(replacement_data)->history;
                assert(std::static_pointer_cast<LRUKReplData>(replacement_data)->lastTouchTick >= history[0]);
                Tick correl_period = std::static_pointer_cast<LRUKReplData>(replacement_data)->lastTouchTick - history[0];

                for (int i = 1; i < history.size(); i++)
                {
                    history[i] = history[i - 1] + correl_period;
                }

                history[0] = cur_tick;
                std::static_pointer_cast<LRUKReplData>(replacement_data)->history = history;
            }
            std::static_pointer_cast<LRUKReplData>(replacement_data)->lastTouchTick = cur_tick;
        }

        void
        LRUK::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // Set last touch timestamp
            std::static_pointer_cast<LRUKReplData>(
                replacement_data)
                ->lastTouchTick = curTick();

            std::vector<Tick> history(k, Tick(0));
            history[0] = curTick();

            std::static_pointer_cast<LRUKReplData>(
                replacement_data)
                ->history = history;
        }

        ReplaceableEntry *
        LRUK::getVictim(const ReplacementCandidates &candidates) const
        {
            // There must be at least one replacement candidate
            assert(candidates.size() > 0);

            // Visit all candidates to find victim
            Tick cur_tick = curTick();
            ReplaceableEntry *victim = NULL;
            for (const auto &candidate : candidates)
            {
                // Update victim entry if necessary
                Tick last_tick = std::static_pointer_cast<LRUKReplData>(
                                     candidate->replacementData)
                                     ->lastTouchTick;
                std::vector<Tick> history = std::static_pointer_cast<LRUKReplData>(
                                                candidate->replacementData)
                                                ->history;
                assert(cur_tick >= last_tick);        
                if (cur_tick - last_tick > CPR)
                {
                    if (victim == NULL)
                    {
                        victim = candidate;
                    }
                    else
                    {
                        if (history[k-1] < std::static_pointer_cast<LRUKReplData>(
                                                victim->replacementData)
                                                ->history[k-1])
                        {
                            victim = candidate;
                        }
                        else if (history[k-1] == std::static_pointer_cast<LRUKReplData>(
                                                victim->replacementData)
                                                ->history[k-1])
                        {
                            if (last_tick < std::static_pointer_cast<LRUKReplData>(
                                                victim->replacementData)
                                                ->lastTouchTick)
                                victim = candidate;
                        }
                        
                    }
                }
            }

            return victim;
        }

        std::shared_ptr<ReplacementData>
        LRUK::instantiateEntry()
        {
            return std::shared_ptr<ReplacementData>(new LRUKReplData(k));
        }

    } // namespace replacement_policy
} // namespace gem5
