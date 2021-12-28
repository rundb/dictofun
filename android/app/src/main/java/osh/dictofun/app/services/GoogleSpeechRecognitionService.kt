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

import android.content.Context
import android.util.Log
import com.google.auth.oauth2.GoogleCredentials
import com.google.cloud.speech.v1.*
import com.google.common.collect.Lists.newArrayList
import com.google.protobuf.ByteString
import java.io.File
import java.nio.file.Files


class GoogleSpeechRecognitionService(context: Context) : ISpeechRecognitionService {
    private val credentials: GoogleCredentials = GoogleCredentials.fromStream(context.assets.open("not-secret.json"))
        .createScoped(newArrayList("https://www.googleapis.com/auth/cloud-platform"))

    override fun recognize(file: File) : String? {
        try {
            SpeechClient.create(
                SpeechSettings.newBuilder()
                    .setCredentialsProvider { credentials }
                    .build()
            ).use { speechClient ->
                // The language of the supplied audio
                val languageCode = "en-US"

                // Sample rate in Hertz of the audio data sent
                val sampleRateHertz = 16000

                // Encoding of audio data sent. This sample sets this explicitly.
                // This field is optional for FLAC and WAV audio formats.
                val encoding: RecognitionConfig.AudioEncoding =
                    RecognitionConfig.AudioEncoding.LINEAR16
                val config: RecognitionConfig = RecognitionConfig.newBuilder()
                    .setLanguageCode(languageCode)
//                    .setSampleRateHertz(sampleRateHertz)
                    .setEncoding(encoding)
                    .build()

                val data: ByteArray = Files.readAllBytes(file.toPath())
                val content: ByteString = ByteString.copyFrom(data)
                val audio: RecognitionAudio =
                    RecognitionAudio.newBuilder().setContent(content).build()
                val request: RecognizeRequest =
                    RecognizeRequest.newBuilder().setConfig(config).setAudio(audio).build()
                val response: RecognizeResponse = speechClient.recognize(request)

                for (result in response.resultsList) {
                    // First alternative is the most probable result
                    val alternative: SpeechRecognitionAlternative = result.alternativesList[0]
                    return alternative.transcript
                }
            }
        } catch (exception: Exception) {
            Log.e("RecognitionService", "Failed to create the client due to: $exception")
        }
        return null
    }

    override fun destroy() {
        TODO("Not yet implemented")
    }
}