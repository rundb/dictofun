package com.esrlabs.dictofun.recordings

import com.thoughtbot.expandablerecyclerview.models.ExpandableGroup
import java.io.File

class Recording(private val file: File, items: List<Transcription>?) :
    ExpandableGroup<Transcription>(file.name, items) {
    fun getFile(): File {
        return file
    }
}