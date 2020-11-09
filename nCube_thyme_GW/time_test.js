var moment = require('moment'); 
require('moment-timezone'); 
moment.tz.setDefault("Asia/Seoul"); 

var example = "1,355,7,36.771,126.9297,37,20.7,27.3,32.3,6.77,3.5"

var logs = example.split(',');
console.log(logs);

var date = moment().format('YYYY/MM/DD,HH:mm:ss'); 
console.log(date);
var data = ""

for (var i =0; i<11; i++){
	if (i < 10){
		data += logs[i] + ",";
	}else{
		data += logs[i];	
	}
	if(i == 2){
		data += date + ',';
	}

	console.log(data);
}
