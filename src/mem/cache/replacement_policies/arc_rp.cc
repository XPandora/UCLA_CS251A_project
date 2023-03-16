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
// #include "slist.hh"

namespace gem5
{

    GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
    namespace replacement_policy
    {

        ARC::ARC(const Params &p)
            : Base(p)
        {
        }

        // void ARC::setCurrentAddr(uint64_t current_tag, unsigned int current_index)
        // {
        //     this->current_tag = current_tag;
        //     this->current_index = current_index;
        // }

        // implementing the REPLACE subroutine
        // This routine invalidates instead of deleting the LRU page as in paper,
        // The invalid ones can be deleted later
        bool ARC::REPLACE(BlockPA blockPA) const
        {
            uint32_t T1_len = slist_length(T1);
            bool remove_L1;

            // invalidate the LRU page in T1 and move to MRU position in B
            if (T1_len > 0 && (T1_len > p || (slist_lookup(B2, blockPA) && T1_len == p)))
                remove_L1 = true;
            else
                remove_L1 = false;

            return remove_L1;
        }

        void
        ARC::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
        {
            // Reset last touch timestamp
            std::static_pointer_cast<ARCReplData>(replacement_data)->lastTouchTick = Tick(0);

            // TODO: remove the corresponding node in L1&L2
            // I think removing the node should happen in getVictim? .. will think about it more
            BlockPA blockPA{current_tag, current_index, NULL};
            if (std::static_pointer_cast<ARCReplData>(replacement_data)->status == EntryStatus::inList1)
            {
                slist_look_del(T1, blockPA);
            }
            else if (std::static_pointer_cast<ARCReplData>(replacement_data)->status == EntryStatus::inList2)
            {
                slist_look_del(T2, blockPA);
            }
            else
            {
                // should not invalidate a invalid block
                assert(false);
            }

            std::static_pointer_cast<ARCReplData>(replacement_data)->status = EntryStatus::Invalid;
        }

        void
        ARC::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // touch is always a cache hit
            // data should always in T1 or T2;
            EntryStatus data_status = std::static_pointer_cast<ARCReplData>(replacement_data)->status;
            // bool data_inTop = std::static_pointer_cast<ARCReplData>(replacement_data)->inTop;

            // block data to touch must be valid!
            assert(data_status != EntryStatus::Invalid);
            BlockPA blockPA{current_tag, current_index, NULL};

            if (data_status == EntryStatus::inList1)
            {
                slist_look_del(T1, blockPA);
                slist_add_head(T2, blockPA);
            }
            else if (data_status == EntryStatus::inList2)
            {
                Node* node = slist_lookup(T2, blockPA);
                slist_repl_head(T2, node);
            }
            // Update variables
            std::static_pointer_cast<ARCReplData>(replacement_data)->status = EntryStatus::inList2;
        }

        void
        ARC::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
        {
            // Set last touch timestamp
            std::static_pointer_cast<ARCReplData>(replacement_data)->lastTouchTick = curTick();
            std::static_pointer_cast<ARCReplData>(replacement_data)->tag = current_tag;
            std::static_pointer_cast<ARCReplData>(replacement_data)->index = current_index;

            BlockPA blockPA{current_tag, current_index, NULL};
            assert(slist_lookup(T1, blockPA) == NULL && slist_lookup(T2, blockPA) == NULL);
            if (slist_lookup(B1, blockPA) != NULL)
            {
                // case 1: found in B1
                // move from B1 to MRU in T2
                slist_look_del(B1, blockPA);
                slist_add_head(T2, blockPA);
                std::static_pointer_cast<ARCReplData>(replacement_data)->status = EntryStatus::inList2;
                // assert(*blockPA.status == EntryStatus::inList2);
            }
            else if (slist_lookup(B2, blockPA) != NULL)
            {
                // case 2: found in B2
                // move from B2 to MRU in T2
                slist_look_del(B2, blockPA);
                slist_add_head(T2, blockPA);
                std::static_pointer_cast<ARCReplData>(replacement_data)->status = EntryStatus::inList2;
                // assert(*blockPA.status == EntryStatus::inList2);
            }
            else
            {
                // case 3: not found in any list
                // move to MRU in T1
                slist_add_head(T1, blockPA);
                std::static_pointer_cast<ARCReplData>(replacement_data)->status = EntryStatus::inList1;
                // assert(*blockPA.status == EntryStatus::inList1);
            }
        }

        ReplaceableEntry *
        ARC::getVictim(const ReplacementCandidates &candidates) const
        {
            // Note: routine of handling conflict miss is
            // 1. call getVictim to find a position
            // 2. call reset() to insert new data to the position of victim
            // I only perform operations for victim blocks in getVictim but leave the rest part for reset

            // There must be at least one replacement candidate
            assert(candidates.size() > 0);

            // Visit all candidates to find victim
            ReplaceableEntry *victim = NULL;

            // search for invalid ones first
            for (const auto &candidate : candidates)
            {
                // Update victim entry if necessary
                if (std::static_pointer_cast<ARCReplData>(candidate->replacementData)->status == EntryStatus::Invalid)
                {
                    victim = candidate;
                    return victim;
                }
            }

            BlockPA blockPA{current_tag, current_index, NULL};
            // Adaption
            // use MRU of L1 or L2 as victim
            bool remove_L1;
            if (slist_lookup(B1, blockPA) != NULL)
            {
                // x in B1, (a miss in ARC(c), a hit in DBL(2c)
                p = std::min(c, p + std::max(slist_length(B2) / slist_length(B1), (unsigned int)1));
                remove_L1 = REPLACE(blockPA);
            }
            else if (slist_lookup(B2, blockPA) != NULL)
            {
                // x in B2 (a miss in ARC(c), a hit in DBL(2c))
                p = std::max((unsigned int)0, p - std::max(slist_length(B1) / slist_length(B2), (unsigned int)1));
                remove_L1 = REPLACE(blockPA);
            }
            else
            {
                // x not found in L1 U L2
                // Case A: T1 U B1 has c pages
                if ((slist_length(T1) + slist_length(B1)) == c)
                {
                    // delete LRU in B1, replace
                    if (slist_length(T1) < c)
                    {
                        slist_delete_tail(B1);
                        remove_L1 = REPLACE(blockPA);
                    }
                    // B1 is empty, invalidate LRU page in T1
                    else
                    {
                        remove_L1 = true;
                    }
                }
                // Case B: T1 U B1 has less than C pages
                else
                {
                    int32_t sum = slist_length(T1) + slist_length(T2) + slist_length(B1) + slist_length(B2);
                    if (sum >= c)
                    {
                        if (sum == 2 * c)
                        {
                            slist_delete_tail(B2);
                        }
                        remove_L1 = REPLACE(blockPA);
                    }
                    else
                    {
                        assert(false);
                    }
                }
            }

            if (remove_L1)
            {
                // Check T1 list for the LRU one
                for (const auto &candidate : candidates)
                {
                    if (std::static_pointer_cast<ARCReplData>(candidate->replacementData)->status == EntryStatus::inList1)
                    {
                        if (victim == NULL)
                        {
                            victim = candidate;
                        }
                        else if (std::static_pointer_cast<ARCReplData>(candidate->replacementData)->lastTouchTick <
                                 std::static_pointer_cast<ARCReplData>(victim->replacementData)->lastTouchTick)
                        {
                            victim = candidate;
                        }
                    }
                }

                uint64_t tag = std::static_pointer_cast<ARCReplData>(victim->replacementData)->tag;
                unsigned int index = std::static_pointer_cast<ARCReplData>(victim->replacementData)->index;

                BlockPA victim_block{tag, index, NULL};
                slist_add_head(B1, victim_block);
                slist_look_del(T1, victim_block);
            }
            else
            {
                // Check T2 list for the LRU one
                assert(slist_length(T2) > 0);
                for (const auto &candidate : candidates)
                {
                    if (std::static_pointer_cast<ARCReplData>(candidate->replacementData)->status == EntryStatus::inList2)
                    {
                        if (victim == NULL)
                        {
                            victim = candidate;
                        }
                        else if (std::static_pointer_cast<ARCReplData>(candidate->replacementData)->lastTouchTick <
                                 std::static_pointer_cast<ARCReplData>(victim->replacementData)->lastTouchTick)
                        {
                            victim = candidate;
                        }
                    }
                }

                uint64_t tag = std::static_pointer_cast<ARCReplData>(victim->replacementData)->tag;
                unsigned int index = std::static_pointer_cast<ARCReplData>(victim->replacementData)->index;

                BlockPA victim_block{tag, index, NULL};
                slist_add_head(B2, victim_block);
                slist_look_del(T2, victim_block);
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
