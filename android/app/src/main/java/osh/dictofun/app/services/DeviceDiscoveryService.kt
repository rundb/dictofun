package osh.dictofun.app.services

import android.app.Service
import android.content.Intent
import android.os.IBinder
import android.util.Log
import java.util.*

class DeviceDiscoveryService : Service() {
    private lateinit var mTimer: Timer

    override fun onCreate() {
        super.onCreate()
        mTimer = Timer()

        //mTimer.scheduleAtFixedRate(this, 0, 10000)
    }

    override fun onBind(intent: Intent): IBinder {
        TODO("Return the communication channel to the service.")
    }

    fun run() {
        TODO("Not yet implemented")
        Log.i("DeviceDiscovery","DeviceDescoveryService.run()")
    }


}