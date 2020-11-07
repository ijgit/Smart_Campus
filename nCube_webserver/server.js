'use strict';
var http = require('http');
var fs = require('fs');
var port = process.env.PORT || 6060;
var google_func_list_28;
var google_func_list_31;
var google_func_list_67;
var google_func_list_47;
var array;


fs.readFile('index.html', function (err, data) {
    array = data.toString().split("\n");
});

http.createServer(function (req, res) {
    fs.readFile('28_point.txt', function (err, data) {
        if(data != undefined)
            google_func_list_28 = data.toString().split("\n");
        
        fs.readFile('31_point.txt', function (err, data) {
            if(data != undefined)
                google_func_list_31 = data.toString().split("\n");

            res.writeHead(200);
            var point_func_idx = 0;
            for (var i in array) {

                if (array[i].indexOf('device_1') !== -1) {
                    for (var j in google_func_list_28)
                        res.write(google_func_list_28[j]);
                    res.write("\r\n");
                    continue;
                }
                else if (array[i].indexOf('device_2') !== -1) {
                    for (var k in google_func_list_31)
                        res.write(google_func_list_31[k]);
                    res.write("\r\n");
                    continue;
                }

                res.write(array[i]);
            }
            res.end();
        });
    });

}).listen(port);