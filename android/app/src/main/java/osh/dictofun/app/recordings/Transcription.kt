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