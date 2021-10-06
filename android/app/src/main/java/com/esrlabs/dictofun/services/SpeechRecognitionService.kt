package com.esrlabs.dictofun.services

import java.io.File

interface ISpeechRecognitionService {

    fun recognize(file: File): String?

    fun destroy()
}