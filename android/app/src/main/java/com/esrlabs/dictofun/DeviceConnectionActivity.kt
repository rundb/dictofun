package com.esrlabs.dictofun

import android.Manifest
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.companion.AssociationRequest
import android.companion.BluetoothDeviceFilter
import android.companion.CompanionDeviceManager
import android.content.*
import android.content.pm.PackageManager
import android.graphics.BlendMode
import android.location.LocationManager
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.ParcelUuid
import android.util.Log
import androidx.activity.result.IntentSenderRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContentProviderCompat.requireContext
import androidx.core.content.ContextCompat
import androidx.core.location.LocationManagerCompat
import java.util.*
import java.util.regex.Pattern

private const val LOCATION_PERMISSION_REQUEST_CODE = 2

class DeviceConnectionActivity : AppCompatActivity() {
    val REQUEST_ENABLE_BT = 100
    private val REQUEST_CODE_BACKGROUND: Int = 5451
    private val SELECT_DEVICE_REQUEST_CODE = 101
    private var isPairing = false
    private var currentBluetoothDevice: BluetoothDevice? = null

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


//        if (bluetoothAdapter.isEnabled == false) {
//            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
//            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
//        }

        scanDevices()

    }

    private val bleScanner by lazy {
        bluetoothAdapter.bluetoothLeScanner
    }

    // From the previous section:
    private val bluetoothAdapter: BluetoothAdapter by lazy{
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter
    }

    private val scanResults = mutableListOf<ScanResult>()

    fun Context.hasPermission(permissionType: String): Boolean {
        return ContextCompat.checkSelfPermission(this, permissionType) ==
                PackageManager.PERMISSION_GRANTED
    }
    val isLocationPermissionGranted
        get() = hasPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION)

    private val locationManager: LocationManager by lazy {
        getSystemService(Context.LOCATION_SERVICE) as LocationManager
    }

    @RequiresApi(Build.VERSION_CODES.Q)
    private fun requestBackgroundPermission() {
        if (!LocationManagerCompat.isLocationEnabled(locationManager)) {
            // Start Location Settings Activity, you should explain to the user why he need to enable location before.
            startActivity(Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS))
        }

        val hasBackgroundLocationPermission = ActivityCompat.checkSelfPermission(
            this,
            Manifest.permission.ACCESS_BACKGROUND_LOCATION
        ) == PackageManager.PERMISSION_GRANTED

        if (hasBackgroundLocationPermission) {
            // handle location update
        } else {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(Manifest.permission.ACCESS_BACKGROUND_LOCATION), REQUEST_CODE_BACKGROUND
            )
        }
    }

    fun scanDevices() {
        Log.i("scan", "location permission: ${isLocationPermissionGranted}")
        if (isLocationPermissionGranted == false)
        {
            Log.w("scan", "permission is not granted, requesting")
            requestBackgroundPermission()
        }
        // Implement scan according to https://punchthrough.com/android-ble-guide/
        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_BALANCED)
            .build()
        bleScanner.startScan(null, scanSettings, scanCallback)

//        val deviceFilter: BluetoothDeviceFilter = BluetoothDeviceFilter.Builder()
//            .setNamePattern(Pattern.compile("dictofun*"))
////            .addServiceUuid(ParcelUuid(UUID(0x123abcL, -1L)), null)
//            .build()
//
//        val pairingRequest: AssociationRequest = AssociationRequest.Builder()
//            .addDeviceFilter(deviceFilter)
//            .setSingleDevice(false)
//            .build()
//
//        val deviceManager: CompanionDeviceManager by lazy {
//            getSystemService(Context.COMPANION_DEVICE_SERVICE) as CompanionDeviceManager
//        }
//
//        deviceManager.associate(pairingRequest,
//            object : CompanionDeviceManager.Callback() {
//                override fun onDeviceFound(chooserLauncher: IntentSender) {
//                    startIntentSenderForResult(
//                        chooserLauncher,
//                        SELECT_DEVICE_REQUEST_CODE,
//                        null,
//                        0,
//                        0,
//                        0
//                    )
//                }
//
//                override fun onFailure(error: CharSequence?) {
//                    Log.e("deviceManager", "Unexpected error: $error")
//                }
//            }, null)
//    }
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val indexQuery = scanResults.indexOfFirst { it.device.address == result.device.address }
            if (indexQuery != -1) { // A scan result already exists with the same address
                scanResults[indexQuery] = result
            } else {
                with(result.device) {
                    Log.i("ScanCallback", "Found BLE device! Name: ${name ?: "Unnamed"}, address: $address")
                }
                scanResults.add(result)
            }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.e("ScanCallback", "onScanFailed: code $errorCode")
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            SELECT_DEVICE_REQUEST_CODE -> when (resultCode) {
                Activity.RESULT_OK -> {
                    // The user chose to pair the app with a Bluetooth device.
                    val deviceToPair: BluetoothDevice? =
                        data?.getParcelableExtra(CompanionDeviceManager.EXTRA_DEVICE)
                    Log.i("onActivityResult", "device to pair address: ${deviceToPair?.address}")
                    deviceToPair?.let { device ->
                        Log.i("onActivityResult", "creating a bond")
                        val bondCreationResult = device.createBond()
                        if (bondCreationResult == false)
                        {
                            Log.e("onActivityResult", "Bond creation has failed")
                        }
                        else {
                            Log.i("onActivityResult", "Bond created: ${bondCreationResult}")

                            // Maintain continuous interaction with a paired device.
                            applicationContext.registerReceiver(
                                pairReceiver,
                                IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
                            )
                        }
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
            val prevState = intent?.getIntExtra(
                BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE,
                BluetoothDevice.ERROR
            )

            if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_BONDED) {
                isPairing = false
                Log.i("bond", "bonding completed successfully")
            } else if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_NONE) {
                Log.e("bond", "bonding has failed")
            }
            else
            {
                Log.e("bond", "unknown statuses of bond: ${prevState} and ${state}")
            }

        }
    }

}