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

#include "mem/cache/replacement_policies/arc_rp.hh"

#include <cassert>
#include <memory>

#include "params/ARCRP.hh"
#include "sim/cur_tick.hh"

namespace gem5
{

    GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
    namespace replacement_policy
    {

        ARC::ARC(const Params &p)
            : Base(p)
        {
        }

        void setCurrentAddr(uint64_t current_tag, unsigned int current_index)
        {
            this->current_tag = current_tag;
            this->current_index = current_index;
        }

        void
        ARC::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
        {
            // Reset last touch timestamp
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->lastTouchTick = Tick(0);

            // TODO: remove the corresponding node in L1&L2
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->status = EntryStatus::Invalid;
        }

        void
        ARC::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
        {

            // if it is in list 1
            EntryStatus data_status = replacement_data->status;
            if (data_status == EntryStatus::inList1)
            {
                slist_look_del(T1, replacement_data);
                slist_add_head(T2, replacement_data);
            }
            // if it is in list 2
            else
            {
                slist_repl_head(T2, replacement_data);
            }

            // Move to MRU position in T2
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->inList1 = 0;
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->inTop = 1;
        }

        void
        ARC::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // Set last touch timestamp
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->lastTouchTick = curTick();

            // TODO: this is a new block data, move it to the L1
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->status = EntryStatus::inList1;
        }

        ReplaceableEntry *
        ARC::getVictim(const ReplacementCandidates &candidates) const
        {
            // There must be at least one replacement candidate
            assert(candidates.size() > 0);

            // Visit all candidates to find victim
            ReplaceableEntry *victim = candidates[0];
            for (const auto &candidate : candidates)
            {
                // Update victim entry if necessary
                if (std::static_pointer_cast<ARCReplData>(
                        candidate->replacementData)
                        ->lastTouchTick <
                    std::static_pointer_cast<ARCReplData>(
                        victim->replacementData)
                        ->lastTouchTick)
                {
                    victim = candidate;
                }
            }

            return victim;
        }

        std::shared_ptr<ReplacementData>
        ARC::instantiateEntry()
        {
            return std::shared_ptr<ReplacementData>(new ARCReplData());
        }

    } // namespace replacement_policy
} // namespace gem5
