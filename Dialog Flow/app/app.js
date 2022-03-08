'use strict'
const express = require('express')
const app = express();
const {
    dialogflow,
} = require('actions-on-google')
const awsIot = require('aws-iot-device-sdk')
const bodyParser = require('body-parser')
const http = require("http");
const socketIO = require("socket.io");
const ejs = require("ejs");

let server = http.createServer(app);
let io = socketIO(server);
const cors = require('cors');



var obj;

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(cors());

app.use(express.static("public"));

app.set('view engine', 'ejs');

const googleAssistant = dialogflow({ debug: true })
app.post('/fulfillment', googleAssistant);

var temperature = 0;
var humidity = 0;
var thingsShadow, topic = "";
thingsShadow = awsIot.device({
    keyPath: "data/0326bc11e5-private.pem.key",
    certPath: "data/0326bc11e5-certificate.pem.crt",
    caPath: "data/AmazonRootCA1.pem",
    clientId: "esp8266",
    host: "a1l34b8dami2sc-ats.iot.ap-south-1.amazonaws.com"
})

thingsShadow.on('connect', function () {
    console.log('connect');
    thingsShadow.subscribe('inTopic');
    thingsShadow.subscribe('outTopic');

})

thingsShadow.on('message', function (topic, payload) {
    try {
        obj = JSON.parse(payload.toString());
        temperature = obj.Temperature;
        humidity = obj.Humidity;
        io.on('status added', socket => {
            setInterval(() => {
                socket.emit('values', "hello");
              }, 1000);
        })
    } catch (err) {
        console.log(err);
    }
})

function publishInTopic(device_name, device_status) {
    thingsShadow.publish('inTopic', JSON.stringify({
        "device": device_name,
        "state": device_status
    }))

}

googleAssistant.intent('DeviceControl', (conv, { devicename, devicestatus }) => {
    consle.log(conv + "hello")
    var device_name = devicename.toLowerCase();
    var device_status = devicestatus.toLowerCase();
    publishInTopic(device_name, device_status, conv);
    conv.ask("Turned " + device_status + " " + device_name);
})


googleAssistant.intent('ReadSensor', (conv, { devicename }) => {
    var device_name = devicename;
    conv.ask("Values from the " + device_name + " is: Temperature " + temperature + " and Humidity is: " + humidity + ".");
})


app.get("/", function (req, res) {
    res.render("main");
})

io.on('connect', socket => {
    socket.on('devices', data => {
        publishInTopic(data.device, data.state);
    })
})

io.on('connection', socket => {
    setInterval(() => {
        socket.emit('values',{temperature,humidity});
      }, 1000);
})

server.listen('8080', function () {
    console.log("Port 8080")
})