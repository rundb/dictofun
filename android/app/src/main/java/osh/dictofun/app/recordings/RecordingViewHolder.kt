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