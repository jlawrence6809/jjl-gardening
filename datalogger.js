var http = require('http');
var fs = require('fs');
//hello world

// let bodies = [];

http.createServer(function (r, s) {
	var body = "";
	r.on('readable', function() {
	    body += r.read();
	});
	r.on('end', function() {
	    let time = new Date().getTime();
		try{
			// remove null at end of string
		    body = body.slice(0, -4);
		    body = body.replace(/nan/g, "NaN");
		    body = eval("z = " + body);
		    // bodies.push({ time: time, body: body });
		    console.log(time + ": " + JSON.stringify(body));
		} catch(e){
			console.log(e);
		}
	    s.write("OK");
	    s.end();
	});
}).listen(9615);