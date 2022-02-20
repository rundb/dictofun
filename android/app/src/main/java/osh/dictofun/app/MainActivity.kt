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
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.le.ScanResult
import android.companion.AssociationRequest
import android.companion.BluetoothLeDeviceFilter
import android.companion.CompanionDeviceManager
import android.content.*
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import android.util.Log
import android.view.View
import android.widget.ListView
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.IntentSenderRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.location.LocationManagerCompat
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.recyclerview.widget.RecyclerView
import osh.dictofun.app.recordings.ExpandableRecordingAdapter
import osh.dictofun.app.recordings.Recording
import osh.dictofun.app.recordings.Transcription
import osh.dictofun.app.services.ExternalStorageService
import osh.dictofun.app.services.FileTransferService
import osh.dictofun.app.services.GoogleSpeechRecognitionService
import osh.dictofun.app.services.ISpeechRecognitionService
import java.lang.reflect.Method
import java.nio.ByteBuffer
import java.util.regex.Pattern
import java.util.stream.Collectors.toList


class MainActivity : AppCompatActivity() {
    private val REQUEST_CODE_PERMISSIONS: Int = 1545
    private val NEW_DEVICE_REGISTRATION_ACTIVITY_INTENT: Int = 1547

    private val RECOGNITION_ENABLED = false
    private val RESET_ASSOTIATIONS_ON_STARTUP = false

    companion object {
        private val TAG: String = "MainActivity"
    }

    private val deviceManager: CompanionDeviceManager by lazy {
        getSystemService(Context.COMPANION_DEVICE_SERVICE) as CompanionDeviceManager
    }

    private val locationManager: LocationManager by lazy {
        getSystemService(Context.LOCATION_SERVICE) as LocationManager
    }

    private var externalStorageService: ExternalStorageService? = null
    private var recognitionService: ISpeechRecognitionService? = null

    private var fileTransferService: FileTransferService? = null
    private var expandableRecordingAdapter: ExpandableRecordingAdapter? = null

    private var filesToReceiveCount: Int = 0
    private var currentFileBytesToReceiveLeft: Int = 0;

    private val mServiceConnection = object : ServiceConnection {

        override fun onServiceConnected(componentName: ComponentName, service: IBinder) {
            fileTransferService = (service as FileTransferService.LocalBinder).service
            if (!fileTransferService!!.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth")
                finish()
            }

            if (deviceManager.associations.isEmpty()) {
                return
            }

            // Automatically connects to the device upon successful start-up initialization.
            val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
            if (bluetoothAdapter == null) {
                Log.e("bonds", "Failed to access ble device")
                return
            }
            for (device in bluetoothAdapter.bondedDevices) {
                if (device.name.startsWith("dictofun")) {
                    Log.i(TAG, "Connecting to bonded device with address ${device.address}")
                    fileTransferService!!.connect(device.address)
                }
            }
        }

        override fun onServiceDisconnected(componentName: ComponentName) {
            fileTransferService = null
        }
    }

    private val bleStatusChangeReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent) {
            val action = intent.action
            val mIntent = intent
            val LOG_TAG = "bleStatusChange"

            Log.d(LOG_TAG, "Command: $action")
            if (action == FileTransferService.ACTION_GATT_CONNECTED) {
                Log.i(LOG_TAG, "File Transfer Service connected")
            }

            if (action == FileTransferService.ACTION_GATT_DISCONNECTED) {
                Log.i(LOG_TAG, "File Transfer Service disconnected")
            }

            if (action == FileTransferService.ACTION_GATT_SERVICES_DISCOVERED) {
                // Subscribe to characteristics once BLE services have been discovered.
                fileTransferService?.enableFileSystemInfoNotification()
                Thread.sleep(1_000)
                fileTransferService?.sendCommand(FileTransferService.Command.GetFilesystemInfo)
            }

            if (action == FileTransferService.ACTION_FILESYSTEM_INFO_AVAILABLE) {
                fileTransferService?.enableFileInfoNotification();
                Thread.sleep(1_000)

                // TODO: process fs info available state
                val txValue = intent.getByteArrayExtra(FileTransferService.EXTRA_DATA)

                if (txValue != null) {
                    filesToReceiveCount =
                        ByteBuffer.wrap(txValue.copyOfRange(1, txValue.size).reversedArray()).int
                    if (filesToReceiveCount != 0) {
                        Log.i(LOG_TAG, "Files to receive count == $filesToReceiveCount")
                        // extract next file's size
                        fileTransferService?.sendCommand(FileTransferService.Command.GetFileInfo)
                    }
                }
                else
                {
                    Log.e(LOG_TAG, "FS Info available: NP txValue")
                }
            }

            if (action == FileTransferService.ACTION_FILE_INFO_AVAILABLE) {
                fileTransferService?.enableTXNotification()
                Thread.sleep(1_000)
                val txValue = intent.getByteArrayExtra(FileTransferService.EXTRA_DATA)

                if (txValue != null) {
                    val fileSize =
                        ByteBuffer.wrap(txValue.copyOfRange(1, txValue.size).reversedArray()).int
                    Log.i(TAG, "Starting reception of the next file, $fileSize bytes")
                    if (fileSize != 0) {
                        Log.i(TAG, "New file is ready for reception, size: $fileSize bytes")
                        currentFileBytesToReceiveLeft = fileSize

                        // Request file.
                        externalStorageService?.initNewFileSaving(fileSize)
                        fileTransferService?.sendCommand(FileTransferService.Command.GetFile);
                    }
                    else {
                        Log.e(TAG, "Received file size equal 0, likely an error in FileInfo request")
                    }
                }

                Log.i(TAG, "File info data: ${txValue.contentToString()}")
            }

            if (action == FileTransferService.ACTION_FILE_DATA) {
                val txValue = intent.getByteArrayExtra(FileTransferService.EXTRA_DATA)
                if (txValue != null) {
                    Log.d(TAG, "received: ${txValue.contentToString()}")
                    externalStorageService?.appendToCurrentFile(txValue)?.ifPresent {
                        // Get transcription.
                        Log.i(
                            TAG,
                            "Getting transcription from recognition service: filename=$it"
                        )
                        if (RECOGNITION_ENABLED) {
                            val result =
                                recognitionService?.recognize(externalStorageService!!.getFile(it))
                            Log.i(TAG, "Result: $result")
                            result?.let { it1 ->
                                externalStorageService?.storeTranscription(
                                    it,
                                    it1
                                )
                            }
                        }

                        externalStorageService?.getFile(it)?.let { it1 ->
                            createRecordingAdapter()
                        }
                    }
                    currentFileBytesToReceiveLeft -= (txValue.size);
                    //Log.d(TAG, "Bytes to receive left: ${currentFileBytesToReceiveLeft}")
                    if (currentFileBytesToReceiveLeft == 0) {
                        Log.i( TAG, "Requesting the next file")
                        filesToReceiveCount--
                        if (filesToReceiveCount != 0) {
                            fileTransferService?.sendCommand(FileTransferService.Command.GetFileInfo)
                        }
                    }
                }
            }

            if (action == FileTransferService.DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER) {
                Log.i(TAG, "APP: Invalid BLE service, disconnecting!")
                fileTransferService?.disconnect()
//                externalStorageService?.resetCurrentFile()
            }
        }
    }

    fun createRecordingAdapter() {
        val storageService = externalStorageService!!
        val recordingList = storageService.listRecordings()

        val recyclerView: RecyclerView = findViewById(R.id.recycler_view)

        recyclerView.destroyDrawingCache()
        recyclerView.visibility = ListView.INVISIBLE
        recyclerView.visibility = ListView.VISIBLE

        expandableRecordingAdapter = recordingList.stream()
            .sorted()
            .map { Recording(it, listOf(Transcription(storageService.getTranscription(it.name)))) }
            .collect(toList()).let { ExpandableRecordingAdapter(it) }

        recyclerView.adapter = expandableRecordingAdapter
    }

    fun eraseBonds(view: View)
    {
        // TODO: ask the user to confirm that he wants to remove the bonds
        val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
        if (bluetoothAdapter == null)
        {
            Log.e("bonds", "Failed to access ble device")
            return
        }
        for (device in bluetoothAdapter.bondedDevices)
        {
            Log.i("bond", "bonded: ${device.name}")
            if (device.name.startsWith("dictofun"))
            {
                val m : Method = device.javaClass.getMethod("removeBond")
                m.invoke(device)
            }
        }
    }

    fun isPairedDeviceFound(): Boolean {
        val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()

        if (bluetoothAdapter != null) {
            for (device in bluetoothAdapter.bondedDevices)
            {
                Log.i("bond", "bonded: ${device.name}, address: ${device.address}")
                if (device.name.startsWith("dictofun"))
                {
                    return true;
                }
            }
        }

        return false
    }

    fun startNewDeviceRegistration(){
        val intent = Intent(this, IntroductionActivity::class.java)
        startActivityForResult(intent, NEW_DEVICE_REGISTRATION_ACTIVITY_INTENT)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == NEW_DEVICE_REGISTRATION_ACTIVITY_INTENT) {
            Log.i(TAG, "Continuing with the main interface")
            runMainActivity()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        checkBtPermissions()

        Log.i(TAG, "onCreate()")
        if (!isPairedDeviceFound()) {
            startNewDeviceRegistration()
        }
        else {
            runMainActivity()
        }
    }

    private val requestMultiplePermissions =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { permissions ->
            permissions.entries.forEach {
                Log.d(TAG, "${it.key} = ${it.value}")
            }
        }

    /***
     * On Android 12+ check bluetooth permissions and request in case of absence.
     */
    private fun checkBtPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val hasBtPermissions = permissionPresent(BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED &&
                    permissionPresent(BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED

            if (!hasBtPermissions) {
                requestMultiplePermissions.launch(arrayOf(BLUETOOTH_SCAN, BLUETOOTH_CONNECT))
            }
        }
    }

    private fun permissionPresent(permission: String) = ActivityCompat.checkSelfPermission(
        this, permission)

    fun runMainActivity() {
        externalStorageService = ExternalStorageService(this)
        if (RECOGNITION_ENABLED) {
            recognitionService = GoogleSpeechRecognitionService(this)
        }

        if (RESET_ASSOTIATIONS_ON_STARTUP) {
            deviceManager.associations.forEach(deviceManager::disassociate)
        }

        val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
        if (bluetoothAdapter == null) {
            // Device doesn't support Bluetooth
        }

        createRecordingAdapter()

        if (deviceManager.associations.isNotEmpty()) {
            if (bluetoothAdapter?.isEnabled == false) {
                requestBluetoothEnabling(null)
            }
            Log.i(TAG, "bindFTS without prompting to pair")
            bindFileTransferService()
        } else {
            Log.i(TAG, "bindFTS with pairing")
            val chooseDeviceActivityResult =
                registerForActivityResult(ActivityResultContracts.StartIntentSenderForResult()) {
                    when (it.resultCode) {
                        Activity.RESULT_OK -> {
                            // The user chose to pair the app with a Bluetooth device.
                            val scanResult: ScanResult? =
                                it.data?.getParcelableExtra(CompanionDeviceManager.EXTRA_DEVICE)
                            val deviceToPair = scanResult?.device
                            deviceToPair?.let { _ ->
                                bindFileTransferService()
                            }
                        }
                    }
                }

            if (bluetoothAdapter?.isEnabled == false) {
                requestBluetoothEnabling(chooseDeviceActivityResult)
            } else {
                // perform bluetooth scanning.
                requestBluetoothSelection(chooseDeviceActivityResult)
            }
        }
    }

    private fun requestBluetoothEnabling(chooseDeviceActivityResult: ActivityResultLauncher<IntentSenderRequest>?) {
        val intentBtEnabled = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);

        val enableBtLauncher =
            registerForActivityResult(ActivityResultContracts.StartActivityForResult())
            {
                if (chooseDeviceActivityResult != null) {
                    requestBluetoothSelection(chooseDeviceActivityResult)
                }
            }

        enableBtLauncher.launch(intentBtEnabled)
    }

    private fun makeGattUpdateIntentFilter(): IntentFilter {
        val intentFilter = IntentFilter()
        intentFilter.addAction(FileTransferService.ACTION_GATT_CONNECTED)
        intentFilter.addAction(FileTransferService.ACTION_GATT_DISCONNECTED)
        intentFilter.addAction(FileTransferService.ACTION_GATT_SERVICES_DISCOVERED)
        intentFilter.addAction(FileTransferService.ACTION_FILE_DATA)
        intentFilter.addAction(FileTransferService.ACTION_FILE_INFO_AVAILABLE)
        intentFilter.addAction(FileTransferService.ACTION_FILESYSTEM_INFO_AVAILABLE)
        intentFilter.addAction(FileTransferService.DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
        return intentFilter
    }

    private fun bindFileTransferService() {
        val gattServiceIntent = Intent(this, FileTransferService::class.java)
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE)

        LocalBroadcastManager.getInstance(this)
            .registerReceiver(bleStatusChangeReceiver, makeGattUpdateIntentFilter());
    }

    private fun requestBluetoothSelection(chooseDeviceActivityResult: ActivityResultLauncher<IntentSenderRequest>) {
        // When the app tries to pair with a Bluetooth device, show the
        // corresponding dialog box to the user.
        deviceManager.associate(
            buildAssociationRequest(),
            object : CompanionDeviceManager.Callback() {
                override fun onDeviceFound(chooserLauncher: IntentSender) {
                    val msg = IntentSenderRequest.Builder(chooserLauncher).build()
                    chooseDeviceActivityResult.launch(msg)
                }

                override fun onFailure(error: CharSequence?) {
                    Log.e("test", "Unexpected error: $error")
                }
            },
            null
        )
    }

    private fun buildAssociationRequest(): AssociationRequest {
        // To skip filters based on names and supported feature flags (UUIDs),
        // omit calls to setNamePattern() and addServiceUuid()
        // respectively, as shown in the following  Bluetooth example.
        val deviceFilter: BluetoothLeDeviceFilter = BluetoothLeDeviceFilter.Builder()
            .setNamePattern(Pattern.compile("dictofun*"))
            .build()

        // The argument provided in setSingleDevice() determines whether a single
        // device name or a list of them appears.
        return AssociationRequest.Builder()
            .addDeviceFilter(deviceFilter)
            .setSingleDevice(false)
            .build()
    }
}