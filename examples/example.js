// check [example.ino] first;
// call this file with `nodejs` to retrieve a device hostname.

const ip = "10.62.41.205"; // replace with your device IP sent to the serial port

fetch(`http://${ip}:80`, { method: "POST", body: "!whoami" })
    .then(res => { 
        res.text().then(data => { console.log(data) }); // "ESP32C5LOL"
    });