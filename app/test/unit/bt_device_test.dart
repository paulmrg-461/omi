import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:omi/backend/schema/bt_device/bt_device.dart';
import 'package:omi/services/devices/models.dart';

void main() {
  group('BtDevice Discovery Logic', () {
    test('isOmiDevice returns true for correct Service UUID', () {
      final device = BluetoothDevice(remoteId: const DeviceIdentifier('11:22:33:44:55:66'));
      final adData = AdvertisementData(
        advName: '',
        txPowerLevel: 0,
        connectable: true,
        manufacturerData: {},
        serviceData: {},
        serviceUuids: [Guid(omiServiceUuid)],
        appearance: 0,
      );
      final result = ScanResult(
        device: device,
        advertisementData: adData,
        rssi: -50,
        timeStamp: DateTime.now(),
      );

      expect(BtDevice.isOmiDevice(result), true);
    });

    test('isSupportedDevice returns true for Nameless Omi Device', () {
       final device = BluetoothDevice(remoteId: const DeviceIdentifier('11:22:33:44:55:66'));
      final adData = AdvertisementData(
        advName: '',
        txPowerLevel: 0,
        connectable: true,
        manufacturerData: {},
        serviceData: {},
        serviceUuids: [Guid(omiServiceUuid)],
        appearance: 0,
      );
      final result = ScanResult(
        device: device,
        advertisementData: adData,
        rssi: -50,
        timeStamp: DateTime.now(),
      );

      expect(BtDevice.isSupportedDevice(result), true);
    });

    test('fromScanResult sets default name for Nameless Omi Device', () {
       final device = BluetoothDevice(remoteId: const DeviceIdentifier('11:22:33:44:55:66'));
      final adData = AdvertisementData(
        advName: '',
        txPowerLevel: 0,
        connectable: true,
        manufacturerData: {},
        serviceData: {},
        serviceUuids: [Guid(omiServiceUuid)],
        appearance: 0,
      );
      final result = ScanResult(
        device: device,
        advertisementData: adData,
        rssi: -50,
        timeStamp: DateTime.now(),
      );

      final btDevice = BtDevice.fromScanResult(result);
      expect(btDevice.name, 'Omi Glass');
      expect(btDevice.type, DeviceType.omi);
    });
  });
}
