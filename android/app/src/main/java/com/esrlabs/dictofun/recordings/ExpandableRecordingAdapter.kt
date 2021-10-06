package com.esrlabs.dictofun.recordings

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.esrlabs.dictofun.R
import com.esrlabs.dictofun.services.MediaPlayerService
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