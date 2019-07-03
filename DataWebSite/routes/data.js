var fs = require('fs');
var express = require('express');
var router = express.Router();

const LOG_ROOT = "/Users/jeremy/code/gardening/DataWebSite/testlogs/";


let data = [];
fs.readdirSync(LOG_ROOT, {encoding: 'utf-8'})
	.filter(f => f.match(/^[0-9]*_garden.log/))
	.map(f => fs.readFileSync(LOG_ROOT + f, {encoding: 'utf-8'}))
	.map(s => JSON.parse(s))
	.forEach(arr => arr.forEach(r => data.push(r)));
	/*
	"time": 1562008193853,
		"body": "{id: 0xe0286c12cfa4, a0: 1.85, a3: 1.96, a6: -1.00, a7: -1.00, a4: 0.48, a5: 0.27, t0: 86.98, t3: 93.04, t6: nan, t7: nan, t4: 13.78, t5: -5.30, th: 109, h: 27 }null"
	*/
router.get('/', function(req, res, next) {
	res.send(data);
	// res.send([
	//   	['Year', 'Sales', 'Expenses'],
	// 	['2004',  1000,      400],
	// 	['2005',  1170,      460],
	// 	['2006',  660,       1120],
	// 	['2007',  1030,      540]
	// ]);
});

module.exports = router;
