// for TAS
var net = require('net');
var ip = require('ip');
var fs = require('fs');
var replaceall = require('replaceall');
var moment = require('moment')
require('moment-timezone');
moment.tz.setDefault('Asia/Seoul');


var socket_arr = {};
exports.socket_arr = socket_arr;

var tas_buffer = {};
exports.buffer = tas_buffer;



const serialport = require('serialport');
const readline = require('@serialport/parser-readline');
var portName = '/dev/ttyAMA0';  // Raspberry pi Serial port

var serial = new serialport(portName, {
  baudRate: 9600,
  dataBits: 8,
  parity: 'none',
//  stopBits: 1,
//  flowControl: false
});

const parser = new readline({ delimiter: '\n' });
serial.pipe(parser);

// Read the port data
serial.on("open", () => {
    console.log('serial port open');
});

parser.on('data', data =>{
    payload = data;
    payload = replaceall("a", "/", payload);
    payload = replaceall("b", ":", payload);
    payload = replaceall("c", ",", payload);
    payload = replaceall("d", ".", payload);

    var cnt = payload.split(',')[0];
    
    var date = moment().format('YYYY/MM/DD,HH:mm:ss');
    var addTimePayload = ""
    
    for(var i =0; i<11; i++){
        if(i < 10){
            addTimePayload += logs[i] + ',';
        }else{
            addTimePayload += logs[i];
        }
        if(i == 2){
            addTimePayload += date + ',';
        }
    }
      
    console.log('m0:', addTimePayload);
    
    serialdata_upload_tomobius(cnt, addTimePayload);
    fs.appendFile('log.txt', data+'\n', function(err){
        if(err) throw err;
    });
});





function serialdata_upload_tomobius(cnt, payload) {
    if (sh_state == 'crtci') {
        for (var j = 0; j < conf.cnt.length; j++) {
            if (conf.cnt[j].name == cnt) {
                var content = payload;
                console.log('client ' + cnt + ' ' + content + ' ---->');
                var parent = conf.cnt[j].parent + '/' + conf.cnt[j].name;
                
                try{
                    sh_adn.crtci(parent, j, content, this, function (status, res_body, to, socket) {
                        console.log('x-m2m-rsc : ' + status + ' <----');
                    });
                }
                catch (e) {
                    console.log(e.message);
                    console.log("result msg this");
                }
                
                break;
            }
        }
    }
}



var _server = null;
exports.ready = function tas_ready () {
    if(_server == null) {
        _server = net.createServer(function (socket) {
            console.logk('socket connected');
            socket.id = Math.random() * 1000;
            tas_buffer[socket.id] = '';
            socket.on('data', tas_handler);
            socket.on('end', function() {
                console.log('end');
            });
            socket.on('close', function() {
                console.log('close');
            });
            socket.on('error', function(e) {
                console.log('error ', e);
            });
        });

        _server.listen(conf.ae.tasport, function() {
            console.log('TCP Server (' + ip.address() + ') for TAS is listening on port ' + conf.ae.tasport);
        });
    }
};



function tas_handler (data) {
    // 'this' refers to the socket calling this callback.
    tas_buffer[this.id] += data.toString();
    //console.log(tas_buffer[this.id]);
    var data_arr = tas_buffer[this.id].split('<EOF>');
    if(data_arr.length >= 2) {
        for (var i = 0; i < data_arr.length-1; i++) {
            var line = data_arr[i];
            tas_buffer[this.id] = tas_buffer[this.id].replace(line+'<EOF>', '');
            var jsonObj = JSON.parse(line);
            var ctname = jsonObj.ctname;
            var content = jsonObj.con;

            socket_arr[ctname] = this;

            console.log('----> got data for [' + ctname + '] from tas ---->');

            if (jsonObj.con == 'hello') {
                this.write(line + '<EOF>');
            }
            else {
                if (sh_state == 'crtci') {
                    for (var j = 0; j < conf.cnt.length; j++) {
                        if (conf.cnt[j].name == ctname) {
                            console.log(line);
                            var parent = conf.cnt[j].parent + '/' + conf.cnt[j].name;
                            sh_adn.crtci(parent, j, content, this, function (status, res_body, to, socket) {
                                try {
                                    var to_arr = to.split('/');
                                    var ctname = to_arr[to_arr.length - 1];
                                    var result = {};
                                    result.ctname = ctname;
                                    result.con = status;

                                    console.log('<---- x-m2m-rsc : ' + status + ' <----');
                                    if (status == 5106 || status == 2001 || status == 4105) {
                                        socket.write(JSON.stringify(result) + '<EOF>');
                                    }
                                    else if (status == 5000) {
                                        sh_state = 'crtae';
                                        socket.write(JSON.stringify(result) + '<EOF>');
                                    }
                                    else if (status == 9999) {
                                        socket.write(JSON.stringify(result) + '<EOF>');
                                    }
                                    else {
                                        socket.write(JSON.stringify(result) + '<EOF>');
                                    }
                                }
                                catch (e) {
                                    console.log(e.message);
                                }
                            });
                            break;
                        }
                    }
                }
            }
        }
    }
}


exports.noti = function(path_arr, cinObj) {
    var cin = {};
    cin.ctname = path_arr[path_arr.length-2];
    cin.con = (cinObj.con != null) ? cinObj.con : cinObj.content;
    delete cinObj;
    cinObj = null;

    if(cin.con == '') {
        console.log('---- is not cin message');
    }
    else {
        //console.log(JSON.stringify(cin));
        console.log('<---- send to tas');

        if (socket_arr[path_arr[path_arr.length-2]] != null) {
            socket_arr[path_arr[path_arr.length-2]].write(JSON.stringify(cin) + '<EOF>');
        }
        delete cin;
        cin = null;
    }
};

