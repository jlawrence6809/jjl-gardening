var http = require('http');

http.createServer(function (r, s) {
	var body = "";
	r.on('readable', function() {
	    body += r.read();
	});
	r.on('end', function() {
	    console.log(new Date().toISOString() + ": " + body);
	    s.write("OK"); 
	    s.end();
	});
}).listen(9615);
