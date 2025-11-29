const SophiePedals = {
    service: 'd98e357f-3d21-4669-a17d-9b389d6559e1',
    buttonDown: '4e9ca473-b618-4de5-a0db-bb1c055a5e1c',
    buttonUp: '019f2af2-6401-445b-a52d-8119aca2c5ef'
};

let sp_device = null;
let sp_server = null;
function onButtonDown(buttonIndex){

}
function onButtonUp(buttonIndex){

}
async function connectBluetooth(){
    if (!navigator.bluetooth) {
        console.log('Web Bluetooth API is not available in this browser.');
        return;
    }


    sp_device = await navigator.bluetooth.requestDevice({
        acceptAllDevices: true,
        optionalServices: ['battery_service', SophiePedals.service]
    });


    sp_server = await device.gatt.connect();
    console.log('Connected to GATT server');
}