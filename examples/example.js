// check [example.ino] first
// call this file with node.js to retrieve device hostname
fetch("http://10.62.41.205:80", { method: "POST", body: "!whoami" })
    .then(res => { 
        res.text().then(data => { console.log(data) }); // "ESP32C5LOL"
    });