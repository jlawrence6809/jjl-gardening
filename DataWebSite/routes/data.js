var fs = require('fs');
var express = require('express');
var router = express.Router();

const LOG_ROOT = "/Users/jeremy/code/gardening/DataWebSite/testlogs/";


let GLOBAL_DATA = [];

let refreshData = () => {
	let dataObj = [];
	fs.readdirSync(LOG_ROOT, {encoding: 'utf-8'})
		.filter(f => f.match(/^[0-9]*_garden.log/))
		.map(f => fs.readFileSync(LOG_ROOT + f, {encoding: 'utf-8'}))
		.map(s => JSON.parse(s))
		.forEach(arr => arr.forEach(r => dataObj.push(r)));

	dataObj.forEach( d => {
      try {
        d.fields = eval("x = " + d.body);
      } catch(e) {
      	console.error('bad row!', d);
        return;
      }
    });

    let intIdToHex = (id) => "0x" + id.toString(16);

    // find all devices and data types represented in data
    let tablesData = {};
    let devices = {};
    let idToIdx = {};
    dataObj.forEach( d =>
      Object.keys(d.fields)
        .forEach(k => {
          if(k === 'id'){
            devices[intIdToHex(d.fields.id)] = true;
          } else {
            tablesData[k] = [];
          }
        })
    );

    // fill in tables data with headers
    let headers = ['Time'].concat(Object.keys(devices));
    Object.values(tablesData)
      .forEach( td => td.push(headers) );

    // fill in tables data with data
    dataObj.forEach( d => {
      let idx = headers.indexOf(intIdToHex(d.fields.id));
      Object.keys(tablesData).forEach( td => {
        let tableData = tablesData[td];
        let row = new Array(headers.length).fill(null);
        row[0] = new Date(d.time);
        row[idx] = d.fields[td];
        tableData.push(row);
      });
    });

    GLOBAL_DATA = tablesData;
};
refreshData();
	/*
	"time": 1562008193853,
		"body": "{id: 0xe0286c12cfa4, a0: 1.85, a3: 1.96, a6: -1.00, a7: -1.00, a4: 0.48, a5: 0.27, t0: 86.98, t3: 93.04, t6: nan, t7: nan, t4: 13.78, t5: -5.30, th: 109, h: 27 }null"
	*/
router.get('/', function(req, res, next) {
	res.send(GLOBAL_DATA);
	// res.send([
	//   	['Year', 'Sales', 'Expenses'],
	// 	['2004',  1000,      400],
	// 	['2005',  1170,      460],
	// 	['2006',  660,       1120],
	// 	['2007',  1030,      540]
	// ]);
});

module.exports = router;
