var request = require("request");
var value = process.argv[3];
var con = process.argv[2];
var mobius_url = 'http://203.253.128.161:7579/Mobius/ubicomp/' + con; 
var options = {
    method: 'POST',
    url: mobius_url,
    headers:
    {
        'Postman-Token': 'ca169f84-44ea-4d33-a5ac-f4c5d8be1c01',
        'cache-control': 'no-cache',
        'Content-Type': 'application/vnd.onem2m-res+json; ty=4',
        'X-M2M-Origin': 'Subicomp',
        'X-M2M-RI': '12345',
        Accept: 'application/json'
    },
    body: '{\n    "m2m:cin": {\n        "con": "' + value + '"\n    }\n}'
};

request(options, function (error, response, body) {
    if (error) throw new Error(error);

    console.log(body);
});
