package com.esrlabs.dictofun

import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.le.ScanResult
import android.companion.AssociationRequest
import android.companion.BluetoothDeviceFilter
import android.companion.CompanionDeviceManager
import android.content.*
import android.graphics.BlendMode
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.ParcelUuid
import android.util.Log
import androidx.activity.result.IntentSenderRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContentProviderCompat.requireContext
import java.util.*
import java.util.regex.Pattern

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

        val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
        if (bluetoothAdapter == null) {
            // Device doesn't support Bluetooth
            Log.e("Connection", "Bluetooth adapter is not present")
            return
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
//            .addServiceUuid(ParcelUuid(UUID(0x123abcL, -1L)), null)
            .build()

        val pairingRequest: AssociationRequest = AssociationRequest.Builder()
            .addDeviceFilter(deviceFilter)
            .setSingleDevice(true)
            .build()

        //val deviceManager = requireContext().getSystemService(Context.COMPANION_DEVICE_SERVICE)
        val deviceManager: CompanionDeviceManager by lazy {
            getSystemService(Context.COMPANION_DEVICE_SERVICE) as CompanionDeviceManager
        }

        deviceManager.associate(pairingRequest,
            object : CompanionDeviceManager.Callback() {
                override fun onDeviceFound(chooserLauncher: IntentSender) {
                    startIntentSenderForResult(chooserLauncher, SELECT_DEVICE_REQUEST_CODE, null, 0, 0, 0)
                }

                override fun onFailure(error: CharSequence?) {
                    Log.e("devicieManager", "Unexpected error: $error")
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
                        Log.i("onActivityResult", "creating a bond")
                        device.createBond()
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
            val state = intent?.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR)
            val prevState = intent?.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR)

            if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_BONDED) {
                isPairing = false
                Log.i("bond", "bonding completed successfully")
            }
            else if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_NONE)
            {
                Log.e("bond", "bonding has failed")
            }

        }
    }

}