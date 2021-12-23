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

import android.os.Parcel
import android.os.Parcelable
import android.os.Parcelable.Creator


class Transcription : Parcelable {
    var text: String

    constructor(text: String) {
        this.text = text
    }

    protected constructor(`in`: Parcel) {
        text = `in`.readString()!!
    }

    override fun describeContents(): Int {
        return 0
    }

    override fun writeToParcel(parcel: Parcel, i: Int) {
        parcel.writeString(text)
    }

    companion object {
        @JvmField
        val CREATOR: Creator<Transcription?> = object : Creator<Transcription?> {
            override fun createFromParcel(`in`: Parcel): Transcription {
                return Transcription(`in`)
            }

            override fun newArray(size: Int): Array<Transcription?> {
                return arrayOfNulls(size)
            }
        }
    }
}