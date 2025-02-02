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

/**
 * @file
 * Declaration of a Least Recently Used replacement policy.
 * The victim is chosen using the last touch timestamp.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "slist.hh"

namespace gem5
{

  struct ARCRPParams;

  GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
  namespace replacement_policy
  {

    class ARC : public Base
    {
    protected:
      /** ARC-specific implementation of replacement data. */
      enum class EntryStatus
      {
        Invalid = -1,
        inList1 = 0,
        inList2 = 1
      };

      struct ARCReplData : ReplacementData
      {
        /** Tick on which the entry was last touched. */
        Tick lastTouchTick;

        // True if in first list (either T or B), False if in second
        EntryStatus status;
        // True if in Top list (either first or second), False if in B
        bool inTop;
        uint64_t tag;
        unsigned int index;

        /**
         * Default constructor. Invalidate data.
         */
        ARCReplData() : status(EntryStatus::Invalid), inTop(0), lastTouchTick(0), tag(0), index(0) {}
      };

      // I think we can remove above lastTouchTick?
      unsigned int block_num = 0;
      unsigned int c = 0; // set*way;
      mutable unsigned int p = 0;

      std::vector<std::shared_ptr<Slist>> T1_vec;
      std::vector<std::shared_ptr<Slist>> B1_vec;
      std::vector<std::shared_ptr<Slist>> T2_vec;
      std::vector<std::shared_ptr<Slist>> B2_vec;

      bool REPLACE(BlockPA blockPA) const;

    public:
      typedef ARCRPParams Params;
      ARC(const Params &p);
      ~ARC() = default;

      // void setCurrentAddr(uint64_t current_tag, unsigned int current_index);
      /**
       * Invalidate replacement data to set it as the next probable victim.
       * Sets its last touch tick as the starting tick.
       *
       * @param replacement_data Replacement data to be invalidated.
       */
      void invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
          override;

      /**
       * Touch an entry to update its replacement data.
       * Sets its last touch tick as the current tick.
       *
       * @param replacement_data Replacement data to be touched.
       */
      void touch(const std::shared_ptr<ReplacementData> &replacement_data) const
          override;

      /**
       * Reset replacement data. Used when an entry is inserted.
       * Sets its last touch tick as the current tick.
       *
       * @param replacement_data Replacement data to be reset.
       */
      void reset(const std::shared_ptr<ReplacementData> &replacement_data) const
          override;

      /**
       * Find replacement victim using ARC timestamps.
       *
       * @param candidates Replacement candidates, selected by indexing policy.
       * @return Replacement entry to be replaced.
       */
      ReplaceableEntry *getVictim(const ReplacementCandidates &candidates) const
          override;

      /**
       * Instantiate a replacement data entry.
       *
       * @return A shared pointer to the new replacement data.
       */
      std::shared_ptr<ReplacementData> instantiateEntry() override;
    };

  } // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_ARC_RP_HH__
