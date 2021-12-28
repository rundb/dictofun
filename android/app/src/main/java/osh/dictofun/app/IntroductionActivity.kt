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

package osh.dictofun.app

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager.PERMISSION_GRANTED
import android.graphics.Color
import android.graphics.Color.GRAY
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView
import osh.dictofun.app.R
import java.security.Permission

class IntroductionActivity : AppCompatActivity() {
    private val DEVICE_CONNECTION_ACTIVITY_DONE_INTENT: Int = 2001

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_introduction)
        Log.i("intro", "onCreate")

        fillPermissionsData()
    }

    fun goToDeviceConnectionActivity(view: View) {
        val intent = Intent(this, DeviceConnectionActivity::class.java)
        startActivityForResult(intent, DEVICE_CONNECTION_ACTIVITY_DONE_INTENT)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == DEVICE_CONNECTION_ACTIVITY_DONE_INTENT)
        {
            Log.i("intro", "pairing is complete. Coming back to the main activity")
            finish()
        }
    }

    fun fillPermissionsData() {
        var bleEnabledView: TextView = findViewById(R.id.bluetoothEnabledView) as TextView
        var locationPermissionView: TextView = findViewById(R.id.locationPermissionGrantedView)
        var backgroundPermissionView: TextView = findViewById(R.id.backgroundPermissionGrantedView)

        var nextButton: Button = findViewById(R.id.introduction_next_button) as Button

        var isLocationPermissionGranted =
            checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)
        var isBackgroundPermissionGranted =
            checkSelfPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION)
        var isBleEnabled = BluetoothAdapter.getDefaultAdapter().isEnabled

        if (isBleEnabled) {
            bleEnabledView.text = "Bluetooth is enabled"
        } else {
            bleEnabledView.text = "Bluetooth is disabled"
            bleEnabledView.setTextColor(Color.parseColor("#ff0000"))
        }

        if (isLocationPermissionGranted == PERMISSION_GRANTED) {
            locationPermissionView.setText("Location permission is granted")
        } else {
            locationPermissionView.setText("Location permission is not granted")
            locationPermissionView.setTextColor(Color.parseColor("#ff0000"))
        }

        if (isBackgroundPermissionGranted == PERMISSION_GRANTED) {
            backgroundPermissionView.setText("Background permission is granted")
        } else {
            backgroundPermissionView.setText("Background permission is not granted")
            backgroundPermissionView.setTextColor(Color.parseColor("#ff0000"))
        }

        nextButton.isClickable = isBleEnabled &&
                (isLocationPermissionGranted == PERMISSION_GRANTED) &&
                (isBackgroundPermissionGranted == PERMISSION_GRANTED)
    }
}