var fs = require('fs');
var express = require('express');
var router = express.Router();

// const LOG_ROOT = "/home/pi/logs/";
const LOG_ROOT = "/Users/jeremy/code/gardening/DataWebSite/testlogs/";


let GLOBAL_DATA = [];

let refreshData = () => {
	let dataObj = [];
	fs.readdirSync(LOG_ROOT, {encoding: 'utf-8'})
		.filter(f => f.match(/^[0-9]*_garden.log/))
		.map(f => fs.readFileSync(LOG_ROOT + f, {encoding: 'utf-8'}))
		.map(s => JSON.parse(s))
		.forEach(arr => arr.forEach(r => dataObj.push(r)));

// for local dev
	dataObj.forEach( d => {
      try {
        d.body = eval("x = " + d.body);
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
      Object.keys(d.body)
        .forEach(k => {
          if(k === 'id'){
            devices[intIdToHex(d.body.id)] = true;
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
      let idx = headers.indexOf(intIdToHex(d.body.id));
      Object.keys(tablesData).forEach( td => {
        let tableData = tablesData[td];
        let row = new Array(headers.length).fill(null);
        row[0] = d.time;
        row[idx] = d.body[td];
        tableData.push(row);
      });
    });

    GLOBAL_DATA = tablesData;
};

refreshData();
setInterval(refreshData, 1000*60*5);
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
