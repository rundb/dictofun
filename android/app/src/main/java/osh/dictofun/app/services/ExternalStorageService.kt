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

package osh.dictofun.app.services

import android.annotation.SuppressLint
import android.content.Context
import android.util.Log
import java.io.File
import java.text.SimpleDateFormat
import java.util.*


class ExternalStorageService(context: Context) {
    val dir = context.getExternalFilesDir(null)

    var currentFilename: String? = null
    private var remainingFilesize = 0

    private fun addToFile(filename: String, bytes: ByteArray) {
        val file = File(dir.toString() + "/" + filename)

        if (!file.exists()) {
            file.createNewFile()
        }

        file.appendBytes(bytes)
    }

    fun listRecordings() : MutableList<File> {
        val file = File(dir.toString())
        val listFiles = file.listFiles().filter { it.name.endsWith(".wav") } ?: return arrayListOf()
        return listFiles.toMutableList()
    }

    fun eraseAllRecords() {
        val file = File(dir.toString())
        val listFiles = file.listFiles().filter { it.name.endsWith(".wav") }
        for (file in listFiles)
        {
            val result = file.delete()
            if (!result)
            {
                Log.e("FilesErase", "erase operation has failed")
                return
            }
        }
    }

    fun getTranscription(recordingName: String) : String {
        val file = File(dir.toString())

        val transcription = file.resolve(recordingName.removeSuffix(".wav") + ".txt")
        return if (transcription.exists()) {
            transcription.readText()
        } else {
            "No transcription"
        }
    }

    fun storeTranscription(recordingName: String, text: String) {
        val file = File(dir.toString())

        val transcriptionFile = file.resolve(recordingName.removeSuffix(".wav") + ".txt")

        if (!transcriptionFile.exists()) {
            transcriptionFile.createNewFile()
        }

        transcriptionFile.writeText(text)
    }

    fun getFile(name: String) : File {
        val file = File(dir.toString())
        return file.resolve(name)
    }

    @SuppressLint("SimpleDateFormat")
    fun initNewFileSaving(size: Int) {
        remainingFilesize = size
        currentFilename = "Recording_" + SimpleDateFormat("yyyyMMdd_HHmmss").format(Date()) + ".wav"
        Log.i("StorageService", "New recording $currentFilename")
    }

    fun appendToCurrentFile(bytes: ByteArray): Optional<String> {
        if (currentFilename == null) return Optional.empty()

        if (remainingFilesize > 0) {
            addToFile(currentFilename!!, bytes)
            remainingFilesize -= bytes.size
            //Log.d("StorageService", "Remaining file size ${remainingFilesize}")
        }

        if (remainingFilesize <= 0) {
            val receivedFile = currentFilename
            currentFilename = null
            Log.i("ExternalStorageService", "finished file reception, returning file name")
            return Optional.ofNullable(receivedFile)
        }
        return Optional.empty()
    }

    fun saveSessionPresent(): Boolean {
        return currentFilename != null
    }

    fun resetCurrentFile() {
        remainingFilesize = 0
        currentFilename = null
    }
}
