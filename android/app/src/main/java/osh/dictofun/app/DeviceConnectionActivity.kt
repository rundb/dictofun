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

import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.companion.AssociationRequest
import android.companion.BluetoothDeviceFilter
import android.companion.CompanionDeviceManager
import android.content.*
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.LinearLayout
import osh.dictofun.app.R
import java.util.regex.Pattern

import no.nordicsemi.android.ble.BleManager;
import java.lang.reflect.Method


class DeviceConnectionActivity : AppCompatActivity() {

    val REQUEST_ENABLE_BT = 100
    private val SELECT_DEVICE_REQUEST_CODE = 101
    private var isPairing = false
    private var currentBluetoothDevice : BluetoothDevice? = null

    /**
     * - scan available devices in BLE (and maybe request permissions before starting the activity)
     * - fill the corresponding layout with list of the devices (VERY LIKELY it is just one device)
     *
     * Following this tutorial:
     * https://developer.android.com/guide/topics/connectivity/companion-device-pairing
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_device_connection)
        // TODO: consider filling this list view with data about the discovered devices
        var devicesListView : LinearLayout = findViewById(R.id.connection_device_list)
        devicesListView.removeAllViews()

        val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
        if (bluetoothAdapter == null) {
            // Device doesn't support Bluetooth
            Log.e("Connection", "Bluetooth adapter is not present")
            return
        }

        for (device in bluetoothAdapter.bondedDevices)
        {
            Log.d("bond", "found bonded: ${device.name}")
        }

        if (bluetoothAdapter?.isEnabled == false) {
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
        }

        scanDevices()
    }

    fun scanDevices() {
        val deviceFilter: BluetoothDeviceFilter = BluetoothDeviceFilter.Builder()
            .setNamePattern(Pattern.compile("dictofun*"))
            .build()

        val pairingRequest: AssociationRequest = AssociationRequest.Builder()
            .addDeviceFilter(deviceFilter)
            .setSingleDevice(true)
            .build()

        val deviceManager: CompanionDeviceManager by lazy {
            getSystemService(Context.COMPANION_DEVICE_SERVICE) as CompanionDeviceManager
        }

        deviceManager.associate(pairingRequest,
            object : CompanionDeviceManager.Callback() {
                override fun onDeviceFound(chooserLauncher: IntentSender) {
                    startIntentSenderForResult(chooserLauncher, SELECT_DEVICE_REQUEST_CODE, null, 0, 0, 0)
                }

                override fun onFailure(error: CharSequence?) {
                    Log.e("deviceManager", "Unexpected error: $error")
                }
            }
            , null)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            SELECT_DEVICE_REQUEST_CODE -> when(resultCode) {
                Activity.RESULT_OK -> {
                    // The user chose to pair the app with a Bluetooth device.
                    val deviceToPair: BluetoothDevice? =
                        data?.getParcelableExtra(CompanionDeviceManager.EXTRA_DEVICE)


                    deviceToPair?.let { device ->
                        Log.i("bond", "creating a bond")
                        Log.i("bond", "bond status: ${device.bondState}")

                        val bondCreationResult = device.createBond()
                        if (!bondCreationResult)
                        {
                            Log.e("bond", "failed to create bond")
                        }
                        // Maintain continuous interaction with a paired device.
                        applicationContext.registerReceiver(pairReceiver, IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED))
                    }
                    currentBluetoothDevice = deviceToPair
                }
            }
            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    private val pairReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            Log.i("bond", "bonding onReceive")
            val state = intent?.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR)
            val prevState = intent?.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR)

            if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_BONDED) {
                isPairing = false
                Log.i("bond", "bonding completed successfully")
                finish()
            }
            else if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_NONE)
            {
                Log.e("bond", "bonding has failed")
                finish()
            }

        }
    }

}