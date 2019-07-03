const http = require('http')

setInterval(() => {
	const options = {
	  hostname: '192.168.29.190',
	  port: 80,
	  path: '/',
	  method: 'GET'
	}
	const req = http.request(options, (res) => {
		res.setEncoding('utf8');
	    res.on('data', function (body) {
	       console.log(new Date().getTime() + ": " + body);
	    });
	})

	req.on('error', (error) => {
	  console.error(error);
	})
	req.end();
}, 10*60*1000);