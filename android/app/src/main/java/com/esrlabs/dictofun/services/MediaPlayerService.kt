package com.esrlabs.dictofun.services

import android.media.MediaPlayer
import android.util.Log
import java.io.File
import java.lang.Exception


class MediaPlayerService {
    private val mediaPlayer = MediaPlayer()

    fun playRecord(file: File) {
        if (mediaPlayer.isPlaying) {
            mediaPlayer.stop()
        }

        try {
            mediaPlayer.reset()
            mediaPlayer.setDataSource(file.absolutePath)
            mediaPlayer.prepare()
            mediaPlayer.start()
        } catch (e: Exception) {
            Log.e("MediaPlayer", "Cannot play the record", e)
        }
    }
}