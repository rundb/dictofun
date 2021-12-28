/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package osh.dictofun.app.recordings

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import osh.dictofun.app.R
import osh.dictofun.app.services.MediaPlayerService
import com.thoughtbot.expandablerecyclerview.ExpandableRecyclerViewAdapter
import com.thoughtbot.expandablerecyclerview.listeners.GroupExpandCollapseListener
import com.thoughtbot.expandablerecyclerview.models.ExpandableGroup

class ExpandableRecordingAdapter(groups: MutableList<Recording?>) :
    ExpandableRecyclerViewAdapter<RecordingViewHolder, TranslationViewHolder>(groups) {


    private val mediaPlayerService: MediaPlayerService = MediaPlayerService()

    init {
        this.setOnGroupExpandCollapseListener(object :
            GroupExpandCollapseListener {
            override fun onGroupExpanded(group: ExpandableGroup<*>?) {
                mediaPlayerService.playRecord((group as Recording).getFile())
            }
            override fun onGroupCollapsed(group: ExpandableGroup<*>?) {}
        })
    }

    override fun onCreateGroupViewHolder(parent: ViewGroup, viewType: Int): RecordingViewHolder {
        val view =
            LayoutInflater.from(parent.context).inflate(R.layout.recording_item, parent, false)
        return RecordingViewHolder(view)
    }

    override fun onCreateChildViewHolder(parent: ViewGroup, viewType: Int): TranslationViewHolder {
        val view =
            LayoutInflater.from(parent.context).inflate(R.layout.transcription_item, parent, false)
        return TranslationViewHolder(view)
    }

    override fun onBindChildViewHolder(
        holder: TranslationViewHolder,
        flatPosition: Int,
        group: ExpandableGroup<*>,
        childIndex: Int
    ) {
        val translation = (group as Recording).items[childIndex]
        holder.itemView.setOnClickListener { l: View? ->
            mediaPlayerService.playRecord(group.getFile())
        }
        holder.setTranslation(translation.text)
    }

    override fun onBindGroupViewHolder(
        holder: RecordingViewHolder,
        flatPosition: Int,
        group: ExpandableGroup<*>?
    ) {
        holder.setRecordingName(group!!)
    }
}