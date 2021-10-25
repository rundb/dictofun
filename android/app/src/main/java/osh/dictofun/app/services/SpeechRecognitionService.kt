package osh.dictofun.app.services

import java.io.File

interface ISpeechRecognitionService {

    fun recognize(file: File): String?

    fun destroy()
}