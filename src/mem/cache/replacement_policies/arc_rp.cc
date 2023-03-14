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


	//implementing the REPLACE subroutine
	//This routine invalidates instead of deleting the LRU page as in paper, 
	//The invalid ones can be deleted later
	void
	ARC::REPLACE(const std::shared_ptr<ReplacementData> &replacement_data)
	{

	   uint32_t T1_len = slist_length(T1);
	   ARCReplData *ptr = std::static_pointer_cast<ARCReplData>(replacement_data);
	   //int32_t *repl_data;

	   //invalidate the LRU page in T1 and move to MRU position in B
	   if( (T1_len > 0) && ((T1_len > p) || ( 
		((! ptr->inTop)&&(ptr->status == EntryStatus::inList2) ) && (T1_len == p) )) ){
		std::static_pointer_cast<ARCReplData>(T1.tail->data)->status = EntryStatus::Invalid;

		slist_add_head(B1, T1.tail->data);
          	slist_delete_tail(T1);
	   }
	   else{
		std::static_pointer_cast<ARCReplData>(T2.tail->data)->status = EntryStatus::Invalid;

		slist_add_head(B2, T2.tail->data);
		slist_delete_tail(T2);
	   }

	   //return repl_data
	}

        void
        ARC::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
        {
            // Reset last touch timestamp
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->lastTouchTick = Tick(0);

            // TODO: remove the corresponding node in L1&L2
	    // I think removing the node should happen in getVictim? .. will think about it more
            std::static_pointer_cast<ARCReplData>(
                replacement_data)
                ->status = EntryStatus::Invalid;
        }

        void
        ARC::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
        {

            EntryStatus data_status = std::static_pointer_cast<ARCReplData>(replacement_data)->status;
	    bool data_inTop = std::static_pointer_cast<ARCReplData>(replacement_data)->inTop;	    
	    //case 1
	    //if it is in T1 or T2, there is a hit -> move to MRU in T2
	    if(data_inTop){
	
	            // if it is in list 1
	            if (data_status == EntryStatus::inList1)
	            {
	                slist_look_del(T1, replacement_data);
                	slist_add_head(T2, replacement_data);
        	    }
	            // if it is in list 2
        	    else if (data_status == EntryStatus::inList2)
	            {	
                	slist_repl_head(T2, replacement_data);
                    }  
        	    // Update variables
	            std::static_pointer_cast<ARCReplData>(replacement_data)->status = EntryStatus::inList2;
	            //std::static_pointer_cast<ARCReplData>(replacement_data)->inTop = 1;
	    }
	    //case 2
	    //it is in B1
	    else if( (!data_inTop) && (data_status == EntryStatus::inList1)){
		//TODO: Adaptation step
		//not sure how to call REPLACE, is it like this or do we have to add the std::shared...?
		REPLACE(replacement_data);
		//move from B1 to MRU in T2
		slist_look_del(B1, replacement_data);
		slist_add_head(T2, replacement_data);
	    }
	    //case 3
	    //it is in B2
	    else if( (!data_inTop) && (data_status == EntryStatus::inList2)){
		//TODO: Adaptation step
    		//same question as above
		REPLACE(replacement_data);
                //move from B2 to MRU in T2
		slist_look_del(B1, replacement_data);
		slist_add_head(T2, replacement_data);
	    }
	    //case 4
	    //it is not in any list
	    else{
		//Case A: T1 U B1 has c pages
		if( (slist_length(T1)+slist_length(B1)) == c){
			//delete LRU in B1, replace
			if(slist_length(T1) < c){
				slist_delete_tail(B1);
				REPLACE(replacement_data);
			}
			//B1 is empty, invalidate LRU page in T1
			else{
			std::static_pointer_cast<ARCReplData>(T1.tail->data)->status = EntryStatus::Invalid;
			}
		}
		//Case B: T1 U B1 has less than C pages
		else{
			int32_t sum = slist_length(T1) + slist_length(T2)
			   		+ slist_length(B1) + slist_length(B2);
			if( sum  >= c){
			   if( sum == 2*c){
				   slist_delete_tail(B2);
			   }
			   REPLACE(replacement_data);
			}
		//move to MRU position in T1
		slist_add_head(T1, replacement_data);
		}
	    }
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

	    //TODO: search for invalid ones first
	    

	    //TODO: next, go through the lists to find which one is LRU and in candidates?

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
