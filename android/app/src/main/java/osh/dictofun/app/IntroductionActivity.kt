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

import android.Manifest.permission.*
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager.PERMISSION_GRANTED
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.core.app.ActivityCompat

class IntroductionActivity : AppCompatActivity() {

    companion object {
        private const val TAG : String = "IntroductionActivity"
        private const val DEVICE_CONNECTION_ACTIVITY_DONE_INTENT: Int = 2001
        private const val PERMISSIONS_REQUEST_CODE: Int = 2002
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_introduction)
        Log.i(TAG, "onCreate")

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
            Log.i(TAG, "pairing is complete. Coming back to the main activity")
            finish()
        }
    }

    fun fillPermissionsData() {
        val bleEnabledView: TextView = findViewById(R.id.bluetoothEnabledView) as TextView
        val locationPermissionView: TextView = findViewById(R.id.locationPermissionGrantedView)
        val backgroundPermissionView: TextView = findViewById(R.id.backgroundPermissionGrantedView)

        val nextButton: Button = findViewById(R.id.introduction_next_button) as Button

        val isLocationPermissionGranted =
            checkSelfPermission(ACCESS_COARSE_LOCATION)
        val isBackgroundPermissionGranted =
            checkSelfPermission(ACCESS_BACKGROUND_LOCATION)
        val isBleEnabled = BluetoothAdapter.getDefaultAdapter().isEnabled

        if (isBleEnabled) {
            bleEnabledView.text = "Bluetooth is enabled"
        } else {
            bleEnabledView.text = "Bluetooth is disabled"
            bleEnabledView.setTextColor(Color.parseColor("#ff0000"))
        }

        if (isLocationPermissionGranted == PERMISSION_GRANTED) {
            locationPermissionView.text = "Location permission is granted"
            locationPermissionView.setOnClickListener(null)
        } else {
            locationPermissionView.text = "Location permission is not granted"
            locationPermissionView.setTextColor(Color.parseColor("#ff0000"))
            locationPermissionView.setOnClickListener{
                // FIXME: 1/16/22 Check if we need fine location?
                // How behaves on Android 11 ?
                requestPermissionEnabling(arrayOf(ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION))
            }
        }

        if (isBackgroundPermissionGranted == PERMISSION_GRANTED) {
            backgroundPermissionView.text = "Background permission is granted"
            locationPermissionView.setOnClickListener(null)
        } else {
            backgroundPermissionView.text = "Background permission is not granted"
            backgroundPermissionView.setTextColor(Color.parseColor("#ff0000"))
            backgroundPermissionView.setOnClickListener{
                requestPermissionEnabling(arrayOf(ACCESS_BACKGROUND_LOCATION))
            }
        }

        nextButton.isEnabled = isBleEnabled &&
                (isLocationPermissionGranted == PERMISSION_GRANTED) &&
                (isBackgroundPermissionGranted == PERMISSION_GRANTED)
    }

    private fun requestPermissionEnabling(permissions: Array<String>) {
        ActivityCompat.requestPermissions(this, permissions, PERMISSIONS_REQUEST_CODE)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        // Refresh activity on receiving permission update.
        if (requestCode == PERMISSIONS_REQUEST_CODE) {
            refreshActivity()
        }
    }

    private fun refreshActivity() {
        finish();
        startActivity(getIntent());
    }
}