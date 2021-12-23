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

import android.view.View
import android.widget.TextView
import com.thoughtbot.expandablerecyclerview.models.ExpandableGroup
import com.thoughtbot.expandablerecyclerview.viewholders.ChildViewHolder
import com.thoughtbot.expandablerecyclerview.viewholders.GroupViewHolder


class RecordingViewHolder(itemView: View) : GroupViewHolder(itemView) {
    private val recordingName: TextView = itemView.findViewById(osh.dictofun.app.R.id.recording_name)


    fun setRecordingName(group: ExpandableGroup<*>) {
        recordingName.text = group.title
    }
}

class TranslationViewHolder(itemView: View) : ChildViewHolder(itemView) {
    private val artistName: TextView = itemView.findViewById(osh.dictofun.app.R.id.transcription_item)
    fun setTranslation(text: String) {
        artistName.text = text
    }
}