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

package osh.dictofun.app.services

import android.app.Service
import android.bluetooth.*
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import androidx.localbroadcastmanager.content.LocalBroadcastManager


import android.util.Log
import java.util.*


/**
 * Service for managing connection and data communication with a GATT server hosted on a
 * given Bluetooth LE device.
 */
class FileTransferService : Service() {
    private var mBluetoothManager: BluetoothManager? = null
    private var mBluetoothAdapter: BluetoothAdapter? = null
    private var mBluetoothDeviceAddress: String? = null
    private var mBluetoothGatt: BluetoothGatt? = null
    private var mConnectionState = STATE_DISCONNECTED

    // Implements callback methods for GATT events that the app cares about.  For example,
    // connection change and services discovered.
    private val mGattCallback: BluetoothGattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            val intentAction: String
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                intentAction = ACTION_GATT_CONNECTED
                mConnectionState = STATE_CONNECTED
                mConnectionState = STATE_CONNECTED
                broadcastUpdate(intentAction)
                Log.i(TAG, "Connected to GATT server.")
                // Attempts to discover services after successful connection.
                Log.i(
                    TAG, "Attempting to start service discovery:" +
                            mBluetoothGatt!!.discoverServices()
                )
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                intentAction = ACTION_GATT_DISCONNECTED
                mConnectionState = STATE_DISCONNECTED
                Log.i(TAG, "Disconnected from GATT server.")
                broadcastUpdate(intentAction)
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.w(
                    TAG,
                    "mBluetoothGatt = $mBluetoothGatt"
                )
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED)
            } else {
                Log.w(
                    TAG,
                    "onServicesDiscovered received: $status"
                )
            }
        }

        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            Log.d(TAG, "onCharacteristicRead() is called")
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_FILE_DATA, characteristic)
            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        )
        {
            if (FILE_INFO_CHAR_UUID == characteristic.uuid) {
                broadcastUpdate(ACTION_FILE_INFO_AVAILABLE, characteristic)
            }
            else if (FILESYSTEM_INFO_CHAR_UUID == characteristic.uuid) {
                broadcastUpdate(ACTION_FILESYSTEM_INFO_AVAILABLE, characteristic)
            } else {
                broadcastUpdate(ACTION_FILE_DATA, characteristic)
            }
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            Log.w(TAG, "OnCharWrite $status")
        }

        override fun onDescriptorWrite(
            gatt: BluetoothGatt,
            descriptor: BluetoothGattDescriptor,
            status: Int
        ) {
            Log.w(TAG, "OnDescWrite!!!")

            if (TX_CHAR_UUID == descriptor.characteristic.uuid) {
                subscribeToFilesystemInfoCharacteristic()
            } else if (FILESYSTEM_INFO_CHAR_UUID == descriptor.characteristic.uuid) {
                subscribeToFileInfoCharacteristic()
            }
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            Log.w(TAG, "MTU changed: $mtu")
        }
    }

    private fun subscribeToFileInfoCharacteristic() {
        // When the first notification is set we can set the second
        val fileTransferService = mBluetoothGatt!!.getService(FILE_TRANSFER_SERVICE_UUID)
        val fileInfoCharacteristic = fileTransferService.getCharacteristic(FILE_INFO_CHAR_UUID)
        if (fileInfoCharacteristic == null) {
            showMessage("File Info characteristic not found!")
            broadcastUpdate("subscribe to file info char " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        mBluetoothGatt!!.setCharacteristicNotification(fileInfoCharacteristic, true)
        val descriptor2 = fileInfoCharacteristic.getDescriptor(CCCD)
        descriptor2.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        val result = mBluetoothGatt!!.writeDescriptor(descriptor2)
        if (!result) {
            Log.e(TAG, "Failed to subscribe to file info characteristic")
        }
    }

    private fun subscribeToFilesystemInfoCharacteristic() {
        val fileTransferService = mBluetoothGatt!!.getService(FILE_TRANSFER_SERVICE_UUID)
        val filesystemInfoCharacteristic =
            fileTransferService.getCharacteristic(FILESYSTEM_INFO_CHAR_UUID)
        if (filesystemInfoCharacteristic == null) {
            showMessage("FS Info characteristic not found!")
            broadcastUpdate("subscribe to FS info char" + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        mBluetoothGatt!!.setCharacteristicNotification(filesystemInfoCharacteristic, true)
        val descriptor2 = filesystemInfoCharacteristic.getDescriptor(CCCD)
        descriptor2.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        val result = mBluetoothGatt!!.writeDescriptor(descriptor2)
        if (!result) {
            Log.e(TAG, "Failed to subscribe to FS info characteristic")
        }
    }

    private fun broadcastUpdate(action: String) {
        val intent = Intent(action)
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
    }

    private fun broadcastUpdate(
        action: String,
        characteristic: BluetoothGattCharacteristic
    ) {
        val intent = Intent(action)

        if (TX_CHAR_UUID == characteristic.uuid) {
            intent.putExtra(EXTRA_DATA, characteristic.value)
        }
        else if (FILE_INFO_CHAR_UUID == characteristic.uuid) {
            intent.putExtra(EXTRA_DATA, characteristic.value)
        }
        else if (FILESYSTEM_INFO_CHAR_UUID == characteristic.uuid) {
            Log.i(TAG, "broadcasting an update of FILESYSTEM_INFO_CHAR_UUID characteristic")
            intent.putExtra(EXTRA_DATA, characteristic.value)
        }
        else {
        }
        val broadcastManager = LocalBroadcastManager.getInstance(this)
        broadcastManager.sendBroadcast(intent)
    }

    inner class LocalBinder : Binder() {
        val service: FileTransferService
            get() = this@FileTransferService
    }

    override fun onBind(intent: Intent): IBinder? {
        return mBinder
    }

    override fun onUnbind(intent: Intent): Boolean {
        // After using a given device, you should make sure that BluetoothGatt.close() is called
        // such that resources are cleaned up properly.  In this particular example, close() is
        // invoked when the UI is disconnected from the Service.
        close()
        return super.onUnbind(intent)
    }

    private val mBinder: IBinder = LocalBinder()

    /**
     * Initializes a reference to the local Bluetooth adapter.
     *
     * @return Return true if the initialization is successful.
     */
    fun initialize(): Boolean {
        // For API level 18 and above, get a reference to BluetoothAdapter through
        // BluetoothManager.
        if (mBluetoothManager == null) {
            mBluetoothManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.")
                return false
            }
        }
        mBluetoothAdapter = mBluetoothManager!!.adapter
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.")
            return false
        }
        return true
    }

    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @param address The device address of the destination device.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     * is reported asynchronously through the
     * `BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)`
     * callback.
     */
    fun connect(address: String?): Boolean {
        if (mBluetoothAdapter == null || address == null) {
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.")
            return false
        }

        // Previously connected device.  Try to reconnect.
        if (mBluetoothDeviceAddress != null && address == mBluetoothDeviceAddress && mBluetoothGatt != null) {
            Log.d(TAG, "Trying to use an existing mBluetoothGatt for connection.")
            return if (mBluetoothGatt!!.connect()) {
                mConnectionState = STATE_CONNECTING
                true
            } else {
                false
            }
        }
        Log.d(TAG, "Address: ${address}")
        val device = mBluetoothAdapter!!.getRemoteDevice(address)
        if (device == null) {
            Log.w(TAG, "Device not found.  Unable to connect.")
            return false
        }
        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        //mBluetoothGatt = device.connectGatt(this, false, mGattCallback);
        mBluetoothGatt = device.connectGatt(this, true, mGattCallback)
        Log.d(TAG, "Trying to create a new connection.")
        mBluetoothDeviceAddress = address
        mConnectionState = STATE_CONNECTING
        return true
    }

    val isConnected: Boolean
        get() = mConnectionState == STATE_CONNECTED

    /**
     * Disconnects an existing connection or cancel a pending connection. The disconnection result
     * is reported asynchronously through the
     * `BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)`
     * callback.
     */
    fun disconnect() {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized")
            return
        }
        mBluetoothGatt!!.disconnect()
        mBluetoothGatt!!.close();
    }

    fun requestMtu(mtu: Int) {
        Log.i(TAG, "Requesting 247 byte MTU")
        mBluetoothGatt!!.requestMtu(mtu)
    }

    /**
     * After using a given BLE device, the app must call this method to ensure resources are
     * released properly.
     */
    fun close() {
        if (mBluetoothGatt == null) {
            return
        }
        Log.w(TAG, "mBluetoothGatt closed")
        mBluetoothDeviceAddress = null
        mBluetoothGatt!!.close()
        mBluetoothGatt = null
    }

    /**
     * Request a read on a given `BluetoothGattCharacteristic`. The read result is reported
     * asynchronously through the `BluetoothGattCallback#onCharacteristicRead(android.bluetooth.BluetoothGatt, android.bluetooth.BluetoothGattCharacteristic, int)`
     * callback.
     *
     * @param characteristic The characteristic to read from.
     */
    fun readCharacteristic(characteristic: BluetoothGattCharacteristic?) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized")
            return
        }
        mBluetoothGatt!!.readCharacteristic(characteristic)
    }

    /**
     * Enables or disables notification on a give characteristic.
     *
     * @param characteristic Characteristic to act on.
     * @param enabled If true, enable notification.  False otherwise.
     */
    fun setCharacteristicNotification(
        characteristic: BluetoothGattCharacteristic,
        enabled: Boolean
    ) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt?.setCharacteristicNotification(characteristic, enabled);


        if (FILE_TRANSFER_SERVICE_UUID.equals(characteristic.getUuid())) {
//            val descriptor = characteristic.getDescriptor(
//                    UUID.fromString(Sam.CLIENT_CHARACTERISTIC_CONFIG));
//            descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
//            mBluetoothGatt?.writeDescriptor(descriptor);
        }
    }

    fun enableFileSystemInfoNotification() {
        Log.i(TAG, "enable file system info notification.")
        val fileTransferService = mBluetoothGatt!!.getService(FILE_TRANSFER_SERVICE_UUID)
        if (fileTransferService == null) {
            showMessage("FTS is null!")
            broadcastUpdate("enableFileSystemInfoNotification " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        val filesystemInfoChar = fileTransferService.getCharacteristic(FILESYSTEM_INFO_CHAR_UUID)
        if (filesystemInfoChar == null) {
            showMessage("File Info characteristic not found!")
            broadcastUpdate("enableFileSystemInfoNotification: " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }

        mBluetoothGatt!!.setCharacteristicNotification(filesystemInfoChar, true)
        val descriptor = filesystemInfoChar.getDescriptor(CCCD)
        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        val status = mBluetoothGatt!!.writeDescriptor(descriptor)
        if (!status) {
            Log.e(TAG, "Failed to enable file system info notification")
        }
    }

    fun enableFileInfoNotification() {
        Log.i(TAG, "enable file info notification.")
        val fileTransferService = mBluetoothGatt!!.getService(FILE_TRANSFER_SERVICE_UUID)
        if (fileTransferService == null) {
            showMessage("FTS is null!")
            broadcastUpdate(DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        val fileInfoChar = fileTransferService.getCharacteristic(FILE_INFO_CHAR_UUID)
        if (fileInfoChar == null) {
            showMessage("File Info characteristic not found!")
            broadcastUpdate(DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }

        mBluetoothGatt!!.setCharacteristicNotification(fileInfoChar, true)
        val descriptor = fileInfoChar.getDescriptor(CCCD)
        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        val status = mBluetoothGatt!!.writeDescriptor(descriptor)
        if (!status) {
            Log.e(TAG, "Failed to enable file info notification")
        }
    }

    /**
     * Subscribe to file info characteristic.
     *
     * @return
     */
    fun enableTXNotification() {
        Log.i(TAG, "enable TX notification.")
        val fileTransferService = mBluetoothGatt!!.getService(FILE_TRANSFER_SERVICE_UUID)
        if (fileTransferService == null) {
            showMessage("Rx service not found!")
            broadcastUpdate("enableTxNotification " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }

        val TxChar = fileTransferService.getCharacteristic(TX_CHAR_UUID)
        if (TxChar == null) {
            showMessage("Tx characteristic not found!")
            broadcastUpdate("enableTxNotification " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        mBluetoothGatt!!.setCharacteristicNotification(TxChar, true)
        var descriptor = TxChar.getDescriptor(CCCD)
        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        var status = mBluetoothGatt!!.writeDescriptor(descriptor)
        if (!status) {
            Log.e(TAG, "Failed to enable TX notification")
        }
    }

    fun writeRXCharacteristic(value: ByteArray?) {
        val RxService = mBluetoothGatt!!.getService(FILE_TRANSFER_SERVICE_UUID)
        if (RxService == null) {
            showMessage("Rx service not found!")
            broadcastUpdate("writeRxChar " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        val RxChar = RxService.getCharacteristic(RX_CHAR_UUID)
        if (RxChar == null) {
            showMessage("Rx characteristic not found!")
            broadcastUpdate("writeRxChar " + DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER)
            return
        }
        RxChar.value = value
        RxChar.writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        val status = mBluetoothGatt!!.writeCharacteristic(RxChar)
        Log.d(TAG, "write Rx char - status=$status")
    }

    fun sendCommand(command: Command) {
        Log.d(TAG, "Sending command: $command")
        val pckData = ByteArray(1)
        pckData[0] = (command.ordinal + 1).toByte()

        writeRXCharacteristic(pckData)
    }

    private fun showMessage(msg: String) {
        Log.e(TAG, msg)
    }

    /**
     * Retrieves a list of supported GATT services on the connected device. This should be
     * invoked only after `BluetoothGatt#discoverServices()` completes successfully.
     *
     * @return A `List` of supported services.
     */
    val supportedGattServices: List<BluetoothGattService>?
        get() = if (mBluetoothGatt == null) null else mBluetoothGatt!!.services

    companion object {
        private const val TAG = "fts_service"
        private const val STATE_DISCONNECTED = 0
        private const val STATE_CONNECTING = 1
        private const val STATE_CONNECTED = 2

        const val ACTION_GATT_CONNECTED = "ACTION_GATT_CONNECTED"
        const val ACTION_GATT_DISCONNECTED = "ACTION_GATT_DISCONNECTED"
        const val ACTION_GATT_SERVICES_DISCOVERED = "ACTION_GATT_SERVICES_DISCOVERED"
        const val ACTION_FILE_INFO_AVAILABLE = "ACTION_FILE_INFO_AVAILABLE"
        const val ACTION_FILESYSTEM_INFO_AVAILABLE = "ACTION_FILESYSTEM_INFO_AVAILABLE"
        const val ACTION_FILE_DATA = "ACTION_FILE_DATA"
        const val EXTRA_DATA = "EXTRA_DATA"
        const val DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER = "DEVICE_DOES_NOT_SUPPORT_IMAGE_TRANSFER"

        val TX_POWER_UUID = UUID.fromString("00001804-0000-1000-8000-00805f9b34fb")
        val TX_POWER_LEVEL_UUID = UUID.fromString("00002a07-0000-1000-8000-00805f9b34fb")
        val CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
        val FIRMWARE_REVISON_UUID = UUID.fromString("00002a26-0000-1000-8000-00805f9b34fb")
        val DIS_UUID = UUID.fromString("0000180a-0000-1000-8000-00805f9b34fb")

        val FILE_TRANSFER_SERVICE_UUID = UUID.fromString("03000001-4202-a882-ec11-b10da4ae3ceb")
        val RX_CHAR_UUID = UUID.fromString("03000002-4202-a882-ec11-b10da4ae3ceb")
        val TX_CHAR_UUID = UUID.fromString("03000003-4202-a882-ec11-b10da4ae3ceb")
        val FILE_INFO_CHAR_UUID = UUID.fromString("03000004-4202-a882-ec11-b10da4ae3ceb")
        val FILESYSTEM_INFO_CHAR_UUID = UUID.fromString("03000005-4202-a882-ec11-b10da4ae3ceb")
    }

    enum class Command {
        GetFile,
        GetFileInfo,
        GetFilesystemInfo
    }
}