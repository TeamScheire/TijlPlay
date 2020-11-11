 
bluetooth.onBluetoothDisconnected(function () {
    connected = 0
    basic.showString("D")
})
bluetooth.onBluetoothConnected(function () {
    connected = 1
    basic.showString("C")
})
let connected = 0
basic.showString("D")
bluetooth.startAccelerometerService()
