package osh.dictofun.app

import android.Manifest
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
import osh.dictofun.app.R
import osh.dictofun.app.recordings.ExpandableRecordingAdapter
import osh.dictofun.app.recordings.Recording
import osh.dictofun.app.recordings.Transcription
import osh.dictofun.app.services.ExternalStorageService
import osh.dictofun.app.services.FileTransferService
import osh.dictofun.app.services.GoogleSpeechRecognitionService
import osh.dictofun.app.services.ISpeechRecognitionService
import java.nio.ByteBuffer
import java.util.regex.Pattern
import java.util.stream.Collectors.toList


class MainActivity : AppCompatActivity() {
    private val REQUEST_CODE_BACKGROUND: Int = 1545

    private val RECOGNITION_ENABLED = true
    private val RESET_ASSOTIATIONS_ON_STARTUP = true

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

    private val mServiceConnection = object : ServiceConnection {

        override fun onServiceConnected(componentName: ComponentName, service: IBinder) {
            fileTransferService = (service as FileTransferService.LocalBinder).service
            if (!fileTransferService!!.initialize()) {
                Log.e("MainActivity", "Unable to initialize Bluetooth")
                finish()
            }

            if (deviceManager.associations.isEmpty()) {
                return
            }

            // Automatically connects to the device upon successful start-up initialization.
            fileTransferService!!.connect(deviceManager.associations[0])
        }

        override fun onServiceDisconnected(componentName: ComponentName) {
            fileTransferService = null
        }
    }

    private val bleStatusChangeReceiver: BroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent) {
            val action = intent.action
            val mIntent = intent

            Log.d("TAG", "Command: $action")
            if (action == FileTransferService.ACTION_GATT_CONNECTED) {
                Log.i("TAG", "UART_CONNECT_MSG")
            }

            if (action == FileTransferService.ACTION_GATT_DISCONNECTED) {
                Log.i("TAG", "UART_DISCONNECT_MSG")
            }

            if (action == FileTransferService.ACTION_GATT_SERVICES_DISCOVERED) {
                // Subscribe to characteristics once BLE services have been discovered.
                fileTransferService?.enableTXNotification()
                Thread.sleep(1_000)
                fileTransferService?.sendCommand(FileTransferService.Command.GetFileInfo)
            }

            if (action == FileTransferService.ACTION_FILE_INFO_AVAILABLE) {
                val txValue = intent.getByteArrayExtra(FileTransferService.EXTRA_DATA)

                if (txValue != null) {
                    val fileSize =
                        ByteBuffer.wrap(txValue.copyOfRange(1, txValue.size).reversedArray()).int
                    if (fileSize != 0) {
                        Log.i("MainActivity", "New file is ready, size: $fileSize bytes")

                        // Request file.
                        externalStorageService?.initNewFileSaving(fileSize)
                        fileTransferService?.sendCommand(FileTransferService.Command.GetFile);
                    }
                }

                Log.i("MainActivity", "File info data: ${txValue.contentToString()}")
            }

            if (action == FileTransferService.ACTION_FILE_DATA) {
                val txValue = intent.getByteArrayExtra(FileTransferService.EXTRA_DATA)
                if (txValue != null) {
                    Log.d("MainActivity", "Data available: ${txValue.contentToString()}")
                    externalStorageService?.appendToCurrentFile(txValue)?.ifPresent {
                        // Get transcription.
                        Log.i(
                            "MainActivity",
                            "Getting transcription from recognition service: filename=$it"
                        )
                        if (RECOGNITION_ENABLED) {
                            val result =
                                recognitionService?.recognize(externalStorageService!!.getFile(it))
                            Log.i("MainActivity", "Result: $result")
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
                }
            }

            if (action == FileTransferService.DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER) {
                Log.i("MainActivity", "APP: Invalid BLE service, disconnecting!")
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

    fun isPairedDeviceFound(): Boolean {
        return false
    }

    fun startNewDeviceRegistration(){
        val intent = Intent(this, IntroductionActivity::class.java)
        startActivity(intent)
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        if (!isPairedDeviceFound())
        {
            startNewDeviceRegistration()
        }
        else {

            externalStorageService = ExternalStorageService(this)
            recognitionService = GoogleSpeechRecognitionService(this) as ISpeechRecognitionService

            if (RESET_ASSOTIATIONS_ON_STARTUP) {
                deviceManager.associations.forEach(deviceManager::disassociate)
            }

            val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
            if (bluetoothAdapter == null) {
                // Device doesn't support Bluetooth
            }

            requestBackgroundPermission()

            createRecordingAdapter()


            if (deviceManager.associations.isNotEmpty()) {
                if (bluetoothAdapter?.isEnabled == false) {
                    requestBluetoothEnabling(null)
                }
                bindFileTransferService()
            } else {
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
            .setNamePattern(Pattern.compile("dictofun_*"))
            .build()

        // The argument provided in setSingleDevice() determines whether a single
        // device name or a list of them appears.
        return AssociationRequest.Builder()
            .addDeviceFilter(deviceFilter)
            .setSingleDevice(false)
            .build()
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
}