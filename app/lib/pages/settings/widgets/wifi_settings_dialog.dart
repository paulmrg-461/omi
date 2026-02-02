import 'package:flutter/material.dart';
import 'package:omi/providers/device_provider.dart';
import 'package:omi/services/services.dart';
import 'package:provider/provider.dart';

class WifiSettingsDialog extends StatefulWidget {
  const WifiSettingsDialog({super.key});

  @override
  State<WifiSettingsDialog> createState() => _WifiSettingsDialogState();
}

class _WifiSettingsDialogState extends State<WifiSettingsDialog> {
  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();
  bool _isLoading = false;
  String? _statusMessage;

  @override
  void dispose() {
    _ssidController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _connect() async {
    setState(() {
      _isLoading = true;
      _statusMessage = null;
    });

    try {
      final provider = context.read<DeviceProvider>();
      final device = provider.connectedDevice;
      
      if (device == null) {
        throw Exception("Device not connected");
      }

      final connection = await ServiceManager.instance().device.ensureConnection(device.id);

      if (connection == null) {
        throw Exception("Could not establish connection to device");
      }

      final result = await connection.setupWifiSync(
        _ssidController.text,
        _passwordController.text,
      );

      if (result.success) {
        if (mounted) {
           setState(() {
             _statusMessage = "Credentials sent! Device is connecting...";
           });
           // Close dialog after delay
           Future.delayed(const Duration(seconds: 2), () {
             if (mounted) Navigator.of(context).pop();
           });
        }
      } else {
         if (mounted) {
           setState(() {
             _statusMessage = "Failed: ${result.errorCode}";
           });
         }
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _statusMessage = "Error: $e";
        });
      }
    } finally {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      backgroundColor: const Color(0xFF1C1C1E),
      title: const Text('Connect Device to WiFi', style: TextStyle(color: Colors.white)),
      content: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          TextField(
            controller: _ssidController,
            style: const TextStyle(color: Colors.white),
            decoration: const InputDecoration(
              labelText: 'SSID',
              labelStyle: TextStyle(color: Colors.grey),
              enabledBorder: UnderlineInputBorder(borderSide: BorderSide(color: Colors.grey)),
            ),
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _passwordController,
            style: const TextStyle(color: Colors.white),
            decoration: const InputDecoration(
              labelText: 'Password',
              labelStyle: TextStyle(color: Colors.grey),
              enabledBorder: UnderlineInputBorder(borderSide: BorderSide(color: Colors.grey)),
            ),
            obscureText: true,
          ),
          if (_statusMessage != null) ...[
            const SizedBox(height: 16),
            Text(
              _statusMessage!,
              style: TextStyle(
                color: _statusMessage!.startsWith('Error') || _statusMessage!.startsWith('Failed') 
                    ? Colors.red 
                    : Colors.green,
                fontSize: 14,
              ),
            ),
          ],
        ],
      ),
      actions: [
        TextButton(
          onPressed: _isLoading ? null : () => Navigator.of(context).pop(),
          child: const Text('Cancel', style: TextStyle(color: Colors.grey)),
        ),
        TextButton(
          onPressed: _isLoading ? null : _connect,
          child: _isLoading 
              ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2))
              : const Text('Connect', style: TextStyle(color: Colors.blue)),
        ),
      ],
    );
  }
}
