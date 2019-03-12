var mqtt = require('mqtt');
var mqttClient = mqtt.connect('mqtt://localhost:1883');
exports.mqttC = mqttClient
var moment = require('moment');
var rpting = require('./reporting.js')
var math = require('math');
var varItem = require('../credentials/variables.js');
const sqlite3 = require('sqlite3').verbose();
const outlet = require('../data/outlet.js')
var nodemailer = require('nodemailer'); 
var transporter = nodemailer.createTransport({
	service: 'gmail',
	auth:{
		user: 'ptutm.jameslsk@gmail.com',
		pass: 'james009'
	}
});

var myRunRecord = {}
var pricing_data = {}
var CoinOut = 0;
var CoinIn = 0;
var detOut = 0;
var softOut = 0;
var begOut = 0;
var lockedLow = 0;
var lockedHigh = 0;
var Activity = 0;
var detCount = 0;
var softCount = 0;
var begCount = 0;
var doneTest = 0;

/////////////////////////////////////
///// sqlite initialization ///////
/////////////////////////////////////
let db = new sqlite3.Database('./mydb/laundry.db', sqlite3.OPEN_READWRITE, (err) => {
  if (err) {
    console.error(err.message);
  }
  console.log('Connected to the laundry database.');
});

//////////////////////////////////////////////////////////////
// Sending the credit to the ESP which received from Wechat///
//////////////////////////////////////////////////////////////

module.exports.onMachine = function (machine_no, money) {
	money_str = money.toString()
	mqttClient.publish(machine_no, money_str)
}

module.exports.sendTime = function () {
	var timeNow = Date.now()
	console.log(timeNow);
	var timeStr = timeNow.toString();
	mqttClient.publish("timeUpdate", timeStr)
}

setTimeout(function() {
	exports.sendTime();
}, 10000);

/////////////////////////////////
//// MQTT subscription //////////
/////////////////////////////////

module.exports.mqttCN = function () {
	mqttClient.on('connect', function () {
		mqttClient.subscribe('connectivity/+')
		mqttClient.subscribe('Lock/+')
		mqttClient.subscribe('coinIn/+')
		mqttClient.subscribe('detDrop/+')
		mqttClient.subscribe('versionFeed/+')
		mqttClient.subscribe('TestChecked')
	})
}


//////////////////////////////////
///// MQTT subscription event ////
//////////////////////////////////

module.exports.mqttSUB = function (VI, pricing) {	
	mqttClient.on('message', function (topic, message) {
		// To check the connectivity of each devices 
		if (topic.match(/versionFeed/g)) {
               		var pattern = /versionFeed\/([0-9a-zA-Z_]+)/i
               		var mchNo = topic.replace(pattern, "$1")
               		//console.log("Current version of the firmware of " + mchNo + " is " + message.toString())
               		VI[mchNo].version = message.toString()
               		exports.updatetotal(mchNo, "version", VI[mchNo].version)
               		console.log("Simulation Test started.")
               		CoinIn = CoinOut = detOut = softOut = begOut = doneTest = Activity = 0;
               		checkTestComplete();
		}
		if (doneTest == 0) {
			if (topic.match(/connectivity/g)) {
				var pattern = /connectivity\/([0-9a-zA-Z_]+)/i
				var mchNo = topic.replace(pattern, "$1")
				if (message.toString() == "ON") {
					VI[mchNo].active = true
					//console.log("Bee~")
				} else if (message.toString() == "OFF") {
					console.log("devices "+VI[mchNo].machineName+" disconnected");
					VI[mchNo].active = false
				}
			// To check whether the machine is running or not 
			} else if (topic.match(/Lock/g)) {
				db.serialize(function() {
					let sql = 'SELECT * FROM pricing WHERE outlet = ?';
					db.all(sql, [outlet.name], function(err, row) {
						if (err) {
				    		return console.error(err.message);
				  		}
				  		pricing_data = Object.assign({}, row[0]);
				  		//console.log(pricing_data)
					})
					var pattern = /Lock\/([0-9a-zA-Z_]+)/i
					var mchNo = topic.replace(pattern, "$1")
					// condition for dex double dryer 
					if (VI[mchNo].typeOfMachine.match(/dex_dryer_double/g)) {
						if (message.toString() == "Locked1") {
							VI[mchNo].lockCounter.upper++
							if (VI[mchNo].wechatPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].wechatPaid
							}
							if (VI[mchNo].manualPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].manualPaid
							}
							if (VI[mchNo].lockCounter.upper == 2 ) {
								VI[mchNo].locked.upper = true	
								//(mchNo, cpd, cp, wpd, wp, mpd, mp, apd, ap, std, st, ld, l, tob)
								VI[mchNo].coinPaidTop = VI[mchNo].coinPaid
								VI[mchNo].wechatPaidTop = VI[mchNo].wechatPaid
								VI[mchNo].manualPaidTop = VI[mchNo].manualPaid
								VI[mchNo].amountPaidTop = VI[mchNo].amountPaid
								VI[mchNo].startTime.upper = moment().format("DD/MM/YYYY HH:mm:ss")
								//(mchNo, cpd, cp, wpd, wp, mpd, mp, apd, ap, std, st, ld, l, tob)
								exports.updateTotalDoubleLocked(mchNo, VI[mchNo].coinPaidTop, 0, VI[mchNo].wechatPaidTop, 0, VI[mchNo].manualPaidTop, 0, VI[mchNo].amountPaidTop, 0, VI[mchNo].startTime.upper, 0, 1, 0, "top")
								VI[mchNo].coinPaid = 0
								VI[mchNo].wechatPaid = 0
								VI[mchNo].amountPaid = 0
								VI[mchNo].manualPaid = 0
								console.log("startTime = " + VI[mchNo].startTime.upper)
							} else if (VI[mchNo].lockCounter.upper <= 1) {
								VI[mchNo].locked.upper= false
								exports.updatetotal(mchNo, "lockedTop", 0)
							}
						} else if (message.toString() == "Locked2") {
							VI[mchNo].lockCounter.lower++
							if (VI[mchNo].wechatPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].wechatPaid
							}
							if (VI[mchNo].manualPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].manualPaid
							}
							if (VI[mchNo].lockCounter.lower == 2 ) {
								VI[mchNo].locked.lower = true
								VI[mchNo].coinPaidBot = VI[mchNo].coinPaid
								VI[mchNo].wechatPaidBot = VI[mchNo].wechatPaid
								VI[mchNo].manualPaidBot = VI[mchNo].manualPaid
								VI[mchNo].amountPaidBot = VI[mchNo].amountPaid
								VI[mchNo].startTime.lower = moment().format("DD/MM/YYYY HH:mm:ss")
								VI[mchNo].coinPaid = 0
								VI[mchNo].wechatPaid = 0
								VI[mchNo].amountPaid = 0
								VI[mchNo].manualPaid = 0
								//(mchNo, cpd, cp, wpd, wp, mpd, mp, apd, ap, std, st, ld, l, tob)
								exports.updateTotalDoubleLocked(mchNo, VI[mchNo].coinPaidBot, 0, VI[mchNo].wechatPaidBot, 0, VI[mchNo].manualPaidBot, 0, VI[mchNo].amountPaidBot, 0, VI[mchNo].startTime.lower, 0, 1, 0, "bot")
								console.log("startTime = " + VI[mchNo].startTime.lower)
							} else if (VI[mchNo].lockCounter.lower <= 1) {
								VI[mchNo].locked.lower= false
								exports.updatetotal(mchNo, "lockedBot", 0)
							}
						} else if (message.toString() == "Unlocked1") {
							VI[mchNo].lockCounter.upper = 0
							if (VI[mchNo].wechatPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].wechatPaid
							}
							if (VI[mchNo].manualPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].manualPaid
							} 
							if (VI[mchNo].locked.upper) {
								lockedHigh = 1;
								if (VI[mchNo].amountPaid != 0) {
									if (VI[mchNo].locked.lower) {
										VI[mchNo].coinPaidBot = VI[mchNo].coinPaidBot + VI[mchNo].coinPaid
										VI[mchNo].wechatPaidBot = VI[mchNo].wechatPaidBot + VI[mchNo].wechatPaid
										VI[mchNo].manualPaidBot = VI[mchNo].manualPaidBot + VI[mchNo].manualPaid
										VI[mchNo].amountPaidBot = VI[mchNo].amountPaidBot + VI[mchNo].amountPaid
										VI[mchNo].wechatPaid = VI[mchNo].coinPaid = VI[mchNo].manualPaid = VI[mchNo].amountPaid = 0
									} else {
										VI[mchNo].coinPaidTop = VI[mchNo].coinPaidTop + VI[mchNo].coinPaid
										VI[mchNo].wechatPaidTop = VI[mchNo].wechatPaidTop + VI[mchNo].wechatPaid
										VI[mchNo].manualPaidTop = VI[mchNo].manualPaidTop + VI[mchNo].manualPaid
										VI[mchNo].amountPaidTop = VI[mchNo].amountPaidTop + VI[mchNo].amountPaid
										VI[mchNo].wechatPaid = VI[mchNo].coinPaid = VI[mchNo].manualPaid = VI[mchNo].amountPaid = 0
									}
									//(mchNo, cp, wp, mp, ap, st, l, ml, ms, md, type)
									exports.resetTotal(mchNo, 0, 0, 0, 0, "NA", 0, 0, 0, 0, "single")
								}
								// wait until the firmware update its been confirmed 	
								var expectedRunTime =  VI[mchNo].amountPaidTop * VI[mchNo].oneRunTime
								VI[mchNo].totalRun = VI[mchNo].totalRun + 1
								VI[mchNo].doneTime.upper = moment().format("DD/MM/YYYY HH:mm:ss")
								var date = moment().format("DD/MM/YYYY") 
								var diff_upper = moment(VI[mchNo].doneTime.upper, "DD/MM/YYYY HH:mm:ss").diff(moment(VI[mchNo].startTime.upper, "DD/MM/YYYY HH:mm:ss"));
								var d_upper = moment.duration(diff_upper);
								var timeTaken_upper = [d_upper.hours(), d_upper.minutes(), d_upper.seconds()].join(':')
								rpting.mchRunRecord(myRunRecord, mchNo, VI[mchNo].machineName, "upper" ,VI[mchNo].totalRun, timeTaken_upper, VI[mchNo].coinPaidTop, VI[mchNo].wechatPaidTop, VI[mchNo].manualPaidTop, 0, 0, 0, "SUCCESS", date, VI[mchNo].startTime.upper, VI[mchNo].doneTime.upper)
								//console.log(myRunRecord[mchNo])
								console.log(myRunRecord[mchNo][VI[mchNo].totalRun])
								VI[mchNo].totalManual = VI[mchNo].totalManual + VI[mchNo].manualPaidTop
								VI[mchNo].totalPaid = VI[mchNo].totalPaid + VI[mchNo].amountPaidTop
								VI[mchNo].totalWechat = VI[mchNo].totalWechat + VI[mchNo].wechatPaidTop
								VI[mchNo].totalCoin = VI[mchNo].totalCoin + VI[mchNo].coinPaidTop
								VI[mchNo].totalTime.upper = VI[mchNo].totalTime.upper + math.floor(d_upper.as('minutes'))
								//(mchNo, tm, tp, tw, tc, tt, ttu, ttb, tr)
								exports.updataTotal(mchNo, VI[mchNo].totalManual, VI[mchNo].totalPaid, VI[mchNo].totalWechat, VI[mchNo].totalCoin, 0, VI[mchNo].totalTime.upper, 0, VI[mchNo].totalRun)
								//(mchNo, cp, wp, mp, art, artt, artb, ert, tr) 
								exports.updateAccumulate(mchNo, VI[mchNo].coinPaid, VI[mchNo].wechatPaid, VI[mchNo].manualPaid, 0, math.floor(d_upper.as('minutes')), 0, expectedRunTime, 1)
								rpting.save2csv("chkMachineRun", myRunRecord[mchNo][VI[mchNo].totalRun], rpting.uploadNothing)	
								VI[mchNo].wechatPaidTop = 0
								VI[mchNo].coinPaidTop = 0
								VI[mchNo].amountPaidTop = 0
								VI[mchNo].manualPaidTop = 0
								exports.resetTotal(mchNo, 0, 0, 0, 0, "NA", 0, 0, 0, 0, "doubleTop")
								console.log("doneTime = " + VI[mchNo].doneTime.upper)
								console.log(timeTaken_upper)
								VI[mchNo].locked.upper = false
							} else {
								VI[mchNo].locked.upper = false
								exports.updatetotal(mchNo, "lockedTop", 0)
							}
						} else if (message.toString() == "Unlocked2") {
							VI[mchNo].lockCounter.lower = 0
							if (VI[mchNo].wechatPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].wechatPaid
							}
							if (VI[mchNo].manualPaid != 0) {
								VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].manualPaid
							} 
							if (VI[mchNo].locked.lower) {
								lockedLow = 1;
								if (VI[mchNo].amountPaid != 0) {
									if (VI[mchNo].locked.upper) {
										VI[mchNo].coinPaidTop = VI[mchNo].coinPaidTop + VI[mchNo].coinPaid
										VI[mchNo].wechatPaidTop = VI[mchNo].wechatPaidTop + VI[mchNo].wechatPaid
										VI[mchNo].manualPaidTop = VI[mchNo].manualPaidTop + VI[mchNo].manualPaid
										VI[mchNo].amountPaidTop = VI[mchNo].amountPaidTop + VI[mchNo].amountPaid
										VI[mchNo].wechatPaid = VI[mchNo].coinPaid = VI[mchNo].manualPaid = VI[mchNo].amountPaid = 0
									} else {
										VI[mchNo].coinPaidBot = VI[mchNo].coinPaidBot + VI[mchNo].coinPaid
										VI[mchNo].wechatPaidBot = VI[mchNo].wechatPaidBot + VI[mchNo].wechatPaid
										VI[mchNo].manualPaidBot = VI[mchNo].manualPaidBot + VI[mchNo].manualPaid
										VI[mchNo].amountPaidBot = VI[mchNo].amountPaidBot + VI[mchNo].amountPaid
										VI[mchNo].wechatPaid = VI[mchNo].coinPaid = VI[mchNo].manualPaid = VI[mchNo].amountPaid = 0
									}
									exports.resetTotal(mchNo, 0, 0, 0, 0, "NA", 0, 0, 0, 0, "single")
								}
								var expectedRunTime =  VI[mchNo].amountPaidBot * VI[mchNo].oneRunTime
								VI[mchNo].totalRun = VI[mchNo].totalRun + 1
								VI[mchNo].doneTime.lower = moment().format("DD/MM/YYYY HH:mm:ss")
								var date = moment().format("DD/MM/YYYY")
								var diff_lower = moment(VI[mchNo].doneTime.lower, "DD/MM/YYYY HH:mm:ss").diff(moment(VI[mchNo].startTime.lower, "DD/MM/YYYY HH:mm:ss"));
								var d_lower = moment.duration(diff_lower);
								var timeTaken_lower = [d_lower.hours(), d_lower.minutes(), d_lower.seconds()].join(':')
								rpting.mchRunRecord(myRunRecord, mchNo, VI[mchNo].machineName, "lower", VI[mchNo].totalRun, timeTaken_lower, VI[mchNo].coinPaidBot, VI[mchNo].wechatPaidBot, VI[mchNo].manualPaidBot, 0, 0, 0, "SUCCESS", date, VI[mchNo].startTime.lower, VI[mchNo].doneTime.lower)
								//console.log(myRunRecord[mchNo])
								console.log(myRunRecord[mchNo][VI[mchNo].totalRun])
								VI[mchNo].totalManual = VI[mchNo].totalManual + VI[mchNo].manualPaidBot
								VI[mchNo].totalPaid = VI[mchNo].totalPaid + VI[mchNo].amountPaidBot
								VI[mchNo].totalWechat = VI[mchNo].totalWechat + VI[mchNo].wechatPaidBot
								VI[mchNo].totalCoin = VI[mchNo].totalCoin + VI[mchNo].coinPaidBot
								VI[mchNo].totalTime.lower = VI[mchNo].totalTime.lower + math.floor(d_lower.as('minutes'))
								//(mchNo, tm, tp, tw, tc, tt, ttu, ttb, tr)
								exports.updataTotal(mchNo, VI[mchNo].totalManual, VI[mchNo].totalPaid, VI[mchNo].totalWechat, VI[mchNo].totalCoin, 0, 0, VI[mchNo].totalTime.lower,  VI[mchNo].totalRun)
								//(mchNo, cp, wp, mp, art, artt, artb, ert, tr) 
								exports.updateAccumulate(mchNo, VI[mchNo].coinPaid, VI[mchNo].wechatPaid, VI[mchNo].manualPaid, 0, 0, math.floor(d_lower.as('minutes')), expectedRunTime, 1)
								rpting.save2csv("chkMachineRun", myRunRecord[mchNo][VI[mchNo].totalRun], rpting.uploadNothing)
								VI[mchNo].coinPaidBot = 0
								VI[mchNo].wechatPaidBot = 0
								VI[mchNo].amountPaidBot = 0
								VI[mchNo].manualPaidBot = 0
								exports.resetTotal(mchNo, 0, 0, 0, 0, "NA", 0, 0, 0, 0, "doubleBot")
								console.log("doneTime = " + VI[mchNo].doneTime.lower)
								console.log(timeTaken_lower)
								VI[mchNo].locked.lower = false
							} else {
								VI[mchNo].locked.lower = false
								exports.updatetotal(mchNo, "lockedBot", 0)
							}
						}
					} else {
						if (message.toString() == "Locked") {
							VI[mchNo].lockCounter++
							if (VI[mchNo].lockCounter == 2 ) {
								VI[mchNo].locked = true
								VI[mchNo].startTime = moment().format("DD/MM/YYYY HH:mm:ss")
								exports.updateTotalDoubleLocked(mchNo, 0, 0, 0, 0, 0, 0, 0, 0, 0, VI[mchNo].startTime, 0, 1, "single")
								console.log("startTime = " + VI[mchNo].startTime)
							} else if (VI[mchNo].lockCounter <= 1) {
								VI[mchNo].locked = false
								exports.updatetotal(mchNo, "locked", 0)
							}
						} else if (message.toString() == "Unlocked") {
							VI[mchNo].lockCounter = 0
							if (VI[mchNo].locked) {
								Activity = 1;
								VI[mchNo].totalRun = VI[mchNo].totalRun + 1
								VI[mchNo].doneTime = moment().format("DD/MM/YYYY HH:mm:ss")
								var date = moment().format("DD/MM/YYYY") 
								var diff = moment(VI[mchNo].doneTime, "DD/MM/YYYY HH:mm:ss").diff(moment(VI[mchNo].startTime, "DD/MM/YYYY HH:mm:ss"));
								var d = moment.duration(diff);
								var timeTaken = [d.hours(), d.minutes(), d.seconds()].join(':')
								//// wait until the firmware update its been confirmed 
								if (VI[mchNo].wechatPaid != 0) {
									VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].wechatPaid
									VI[mchNo].amountPaid = VI[mchNo].amountPaid - VI[mchNo].wechatPaid
								}
								if (VI[mchNo].manualPaid != 0) {
									VI[mchNo].coinPaid = VI[mchNo].coinPaid - VI[mchNo].manualPaid
									VI[mchNo].amountPaid = VI[mchNo].amountPaid - VI[mchNo].manualPaid
								} 
								rpting.mchRunRecord(myRunRecord, mchNo, VI[mchNo].machineName, "NA", VI[mchNo].totalRun, timeTaken, VI[mchNo].coinPaid, VI[mchNo].wechatPaid, VI[mchNo].manualPaid, 0, 0, 0, "SUCCESS", date, VI[mchNo].startTime, VI[mchNo].doneTime)
								//console.log(myRunRecord[mchNo])
								console.log(myRunRecord[mchNo][VI[mchNo].totalRun])
								VI[mchNo].totalPaid = VI[mchNo].totalPaid + VI[mchNo].amountPaid
								VI[mchNo].totalWechat = VI[mchNo].totalWechat + VI[mchNo].wechatPaid
								VI[mchNo].totalCoin = VI[mchNo].totalCoin + VI[mchNo].coinPaid
								VI[mchNo].totalTime = VI[mchNo].totalTime + math.floor(d.as('minutes'))
								VI[mchNo].totalManual = VI[mchNo].totalManual + VI[mchNo].manualPaid
								//(mchNo, tm, tp, tw, tc, tt, ttu, ttb, tr)
								exports.updataTotal(mchNo, VI[mchNo].totalManual, VI[mchNo].totalPaid, VI[mchNo].totalWechat, VI[mchNo].totalCoin, VI[mchNo].totalTime, 0, 0,  VI[mchNo].totalRun)
								rpting.save2csv("chkMachineRun", myRunRecord[mchNo][VI[mchNo].totalRun], rpting.uploadNothing)
								if (VI[mchNo].typeOfMachine.match(/Washer/g)) {
									var expectedRunTime = VI[mchNo].oneRunTime
									var pattern = /[0-9]+kg/g
									var mchKg = VI[mchNo].typeOfMachine.match(pattern)
									var mchKgCold = "cold"+mchKg
									var mchKgWarm = "warm"+mchKg
									var mchKgHot = "hot"+mchKg
									var newLine = "\r\n";
									//console.log(mchKgCold+" "+mchKgWarm+" "+mchKgHot)
									//console.log(pricing_data)
									if (VI[mchNo].amountPaid == pricing_data[mchKgCold]) {
										VI[mchNo].coldRun = VI[mchNo].coldRun + 1;
										exports.updatetotal(mchNo, "totalColdRun", VI[mchNo].coldRun)
										exports.updateAccumulative(mchNo, "No_Cold_Run", 1)
									} else if (VI[mchNo].amountPaid == pricing_data[mchKgWarm]) {
										VI[mchNo].warmRun = VI[mchNo].warmRun + 1;
										exports.updatetotal(mchNo, "totalWarmRun", VI[mchNo].warmRun)
										exports.updateAccumulative(mchNo, "No_Warm_Run", 1)
									} else if (VI[mchNo].amountPaid == pricing_data[mchKgHot]) {
										VI[mchNo].hotRun = VI[mchNo].hotRun + 1;
										exports.updatetotal(mchNo, "totalHotRun", VI[mchNo].hotRun)
										exports.updateAccumulative(mchNo, "No_Hot_Run", 1)
									} else {
										VI[mchNo].otherRun = VI[mchNo].otherRun + 1;
										exports.updatetotal(mchNo, "totalOtherRun", VI[mchNo].otherRun)
										exports.updateAccumulative(mchNo, "No_Other_Run", 1)
									}
									exports.updateAccumulative(mchNo, "Expected_Total_Run_Time", expectedRunTime)
								} else if (VI[mchNo].typeOfMachine.match(/dryer/g)) {
									var expectedRunTime =  VI[mchNo].amountPaid * VI[mchNo].oneRunTime
									exports.updateAccumulative(mchNo, "Expected_Total_Run_Time", expectedRunTime)
								}
								//(mchNo, cp, wp, mp, art, artt, artb, ert, tr) 
								exports.updateAccumulate(mchNo, VI[mchNo].coinPaid, VI[mchNo].wechatPaid, VI[mchNo].manualPaid, math.floor(d.as('minutes')), 0, 0, expectedRunTime, 1)
								VI[mchNo].coinPaid = 0
								VI[mchNo].wechatPaid = 0
								VI[mchNo].amountPaid = 0
								VI[mchNo].manualPaid = 0
								exports.resetTotal(mchNo, 0, 0, 0, 0, "NA", 0, 0, 0, 0, "single")
								console.log("doneTime = " + VI[mchNo].doneTime)
								console.log(timeTaken)
								VI[mchNo].locked = false
							} else {
								VI[mchNo].locked = false
								exports.updatetotal(mchNo, "locked", 0)
							}
						}	
					}
					if (lockedLow && lockedHigh) {
						Activity = 1;
						lockedLow = lockedHigh = 0
					}
					console.log(message.toString() + "  " + mchNo)
				})
			// to check the coin drop event 
			} else if (topic.match(/coinIn/g)) {
				CoinIn = 1;
				var pattern = /coinIn\/([0-9a-zA-Z_]+)/i
				var mchNo = topic.replace(pattern, "$1")
				if (VI[mchNo].typeOfMachine == "detergent") {
					if (message.toString() == "coin1") {
						VI[mchNo].coinPaid = VI[mchNo].coinPaid + 1
						VI[mchNo].amountPaid = VI[mchNo].amountPaid + 1
						VI[mchNo].totalCoin = VI[mchNo].totalCoin + 1
						VI[mchNo].totalPaid = VI[mchNo].totalPaid + 1
					} else if (message.toString() == "coin2") {
						VI[mchNo].coinPaid = VI[mchNo].coinPaid + 1
						VI[mchNo].amountPaid = VI[mchNo].amountPaid + 1
						VI[mchNo].totalCoin = VI[mchNo].totalCoin + 1
						VI[mchNo].totalPaid = VI[mchNo].totalPaid + 1
					}
					exports.updateAccumulative(mchNo, "Coin_received", 1);
					//(mchNo, cp, wp, mp, ap, tc, tw, tm, tp, type)
					exports.updateMoney(mchNo, VI[mchNo].coinPaid, 0, 0, 0, VI[mchNo].totalCoin, 0, 0, VI[mchNo].totalPaid, "detCoin")
					console.log(message.toString() + "  " + mchNo + " " + moment().format("DD/MM/YYYY HH:mm:ss"))
				} else {
					VI[mchNo].coinPaid = VI[mchNo].coinPaid + 1
					VI[mchNo].amountPaid = VI[mchNo].amountPaid + 1
					//(mchNo, cp, wp, mp, ap, tc, tw, tm, tp, type)
					exports.updateMoney(mchNo, VI[mchNo].coinPaid, 0, 0, VI[mchNo].amountPaid, 0, 0, 0, 0, "Coin")
					console.log(message.toString() + "  " + mchNo + " " + moment().format("DD/MM/YYYY HH:mm:ss"))
				}
			// to check the det drop event 
			} else if (topic.match(/detDrop/g)) {
				var pattern = /detDrop\/([0-9a-zA-Z_]+)/i
				var mchNo = topic.replace(pattern, "$1")
				if (VI[mchNo].locked == false) {
					console.log("got things dropped")
					VI[mchNo].locked = true;
					VI[mchNo].totalRun = VI[mchNo].totalRun + 1;
					exports.updatetotal(mchNo, "totalRun", VI[mchNo].totalRun)
					var dateNow = moment().format("DD/MM/YYYY");
					var timeNow = moment().format("DD/MM/YYYY HH:mm:ss");
					setTimeout((function (mn) {
						return function() {
							//detRunRecord = function (myDetR, mchCode, name, noOfRun, coinPaid, wechatPaid, manualPaid, det, softnr, lb, date, Time)
							//console.log("finished one round")
							rpting.mchRunRecord(myRunRecord, mn, VI[mn].machineName, "NA", VI[mn].totalRun, 0, VI[mn].coinPaid, VI[mn].wechatPaid, VI[mn].manualPaid, VI[mn].myDet, VI[mn].mySoft, VI[mn].myLb, "SUCCESS", dateNow, timeNow, "NA")
							//rpting.detRunRecord(myDetRecord, mchNo, VI[mchNo].machineName, VI[mchNo].totalRun, VI[mchNo].coinPaid, VI[mchNo].wechatPaid, VI[mchNo].manualPaid, VI[mchNo].amountPaid, VI[mchNo].myDet, VI[mchNo].mySoft, VI[mchNo].myLb, dateNow, timeNow)
							VI[mn].coinPaid = VI[mn].wechatPaid = VI[mn].manualPaid = VI[mn].amountPaid = VI[mn].myDet = VI[mn].mySoft = VI[mn].myLb = 0;
							exports.resetTotal(mn, 0, 0, 0, 0, "NA", 0, 0, 0, 0, "vending")
							VI[mn].locked = false;
							rpting.save2csv("chkMachineRun", myRunRecord[mn][VI[mn].totalRun], rpting.uploadNothing)
						}
					})(mchNo), 180000)
				}
				console.log(message.toString() + "  " + mchNo + " " + moment().format("DD/MM/YYYY HH:mm:ss"))
				if (message.toString() == "DTG_DROP") {
					VI[mchNo].myDet = VI[mchNo].myDet + 1
					VI[mchNo].detergent = VI[mchNo].detergent + 1
					exports.updatetotal(mchNo, "myDet", VI[mchNo].myDet)
					exports.updatetotal(mchNo, "detergent", VI[mchNo].detergent)
					exports.updateAccumulative(mchNo, "totalDet", 1)
					detCount++;
					detOut = 1;
				} else if (message.toString() == "SFTNR_DROP") {
					VI[mchNo].mySoft = VI[mchNo].mySoft + 1
					VI[mchNo].softnr = VI[mchNo].softnr + 1
					exports.updatetotal(mchNo, "mySoft", VI[mchNo].mySoft)
					exports.updatetotal(mchNo, "softnr", VI[mchNo].softnr)
					exports.updateAccumulative(mchNo, "totalSoftnr", 1)
					softCount++;
					softOut = 1;
				} else if (message.toString() == "BEG_DROP") {
					VI[mchNo].myLb = VI[mchNo].myLb + 1
					VI[mchNo].beg = VI[mchNo].beg + 1
					exports.updatetotal(mchNo, "myLb", VI[mchNo].myLb)
					exports.updatetotal(mchNo, "beg", VI[mchNo].beg)
					exports.updateAccumulative(mchNo, "totalBeg", 1)
					begCount++;
					begOut = 1;
				}
				if (detOut && softOut && begOut && detCount == 5 && softCount == 5 && begCount == 5) {
					console.log("All Dropped")
					Activity = 1;
					detOut = softOut = begOut = detCount = softCount = begCount = 0;
				}
			} else if (topic.match(/TestChecked/g)) {
				CoinOut = 1;
				console.log("Coin out Received");
			}
		}
	})
}


function checkTestComplete () {
	if (CoinOut && CoinIn && Activity) {
		console.log("Test Complete, all Tested OK !!!")
		CoinOut = CoinIn = Activity =  0;
		doneTest = 1;
	}
	setTimeout(checkTestComplete, 10000);
}


////////////////////////////////////////////////////////
/////// sqlite command to update/query the database ////
////////////////////////////////////////////////////////


module.exports.updatetotal = function (mchNo, key, value) {
	let data = [value, mchNo];
	let sql = 'UPDATE dailyTotalValue SET '+key+'= ? WHERE MchCode = ?';
	db.run(sql, data, function(err) {
		if (err) {
    		return console.error(err.message);
  		}
  		//console.log("data has been updated")
	});
}

module.exports.updataTotal = function (mchNo, tm, tp, tw, tc, tt, ttu, ttb, tr) {
	let data = [tm, tp, tw, tc, tt, ttu, ttb, tr, mchNo]
	let sql = 'UPDATE dailyTotalValue SET totalManual = ?, totalPaid = ?, totalWechat = ?, totalCoin = ?, totalTime = ?, totalTimeTop = ?, totalTimeBot = ?, totalRun = ? WHERE MchCode = ?'; 
	db.run(sql, data, function(err) {
		if (err) {
    		return console.error(err.message);
  		}
  		//console.log("data has been updated")
	});
}

module.exports.updateAccumulative = function (mchNo, key, value) {
	let tmpkey = key+"_tmp"
	db.serialize(function() {
		let sql = 'SELECT '+key+' '+tmpkey+' FROM uptoDateValue WHERE Machine_Code = ?';
		db.get(sql, [mchNo], function(err, row) {
			//console.log("the data is "+row[tmpkey])
			var total = row[tmpkey] + value
			if (err) {
	    		return console.error(err.message);
	  		}
	  		let sql = 'UPDATE uptoDateValue SET '+key+'= ? WHERE Machine_Code = ?';
			let data = [total, mchNo];
			db.run(sql, data, function(err) {
				if (err) {
	    			return console.error(err.message);
	  			}
	  			//console.log("up2date data has been updated")
			});
		})
	})	
}

module.exports.updateAccumulate = function (mchNo, cp, wp, mp, art, artt, artb, ert, tr) {
	
	db.serialize(function() {
		let sql = 'SELECT Coin_received cr, Wechat_received wr, Manual_payment mp, Actual_Total_Run_Time atrt, Actual_Total_Run_Time_Top atrtt, Actual_Total_Run_Time_Bot atrtb, Expected_Total_Run_Time etrt, Total_Run tr FROM uptoDateValue WHERE Machine_Code = ?';
		db.get(sql, [mchNo], function(err, row) {
			//console.log("the data is "+row[tmpkey])
			console.log(row)
			var total_cr = row.cr + cp;
			var total_wr = row.wr + wp;
			var total_mp = row.mp + mp;
			var total_atrt = row.atrt + art;
			var total_atrtt = row.atrtt + artt;
			var total_atrtb = row.atrtb + artb;
			var total_etrt = row.etrt + ert;
			var total_tr = row.tr + tr;
			if (err) {
	    		return console.error(err.message);
	  		}
	  		let sql2 = 'UPDATE uptoDateValue SET Coin_received = ?, Wechat_received = ?, Manual_payment = ?, Actual_Total_Run_Time = ?, Actual_Total_Run_Time_Top = ?, Actual_Total_Run_Time_Bot = ?, Expected_Total_Run_Time = ?, Total_Run = ? WHERE Machine_Code = ?';
			let data2 = [total_cr, total_wr, total_mp, total_atrt, total_atrtt, total_atrtb, total_etrt, total_tr, mchNo];
			db.run(sql2, data2, function(err) {
				if (err) {
	    			return console.error(err.message);
	  			}
	  			//console.log("up2date data has been updated")
			});
		})
	})	
}

module.exports.updateTotalDoubleLocked = function (mchNo, cpd, cp, wpd, wp, mpd, mp, apd, ap, std, st, ld, l, tob) {
	if (tob == "top") {
		let data = [cpd, cp, wpd, wp, mpd, mp, apd, ap, std, ld, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaidTop = ?, coinPaid = ?, wechatPaidTop = ?, wechatPaid = ?, manualPaidTop = ?, manualPaid = ?, amountPaidTop = ?, amountPaid = ?, startTimeTop = ?, lockedTop = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (tob == "bot") {
		let data = [cpd, cp, wpd, wp, mpd, mp, apd, ap, std, ld, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaidBot = ?, coinPaid = ?, wechatPaidBot = ?, wechatPaid = ?, manualPaidBot = ?, manualPaid = ?, amountPaidBot = ?, amountPaid = ?, startTimeBot = ?, lockedBot = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (tob == "single") {
		let data = [st, l, mchNo]
		let sql = 'UPDATE dailyTotalValue SET startTime = ?, locked = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	}	
}

module.exports.resetTotal = function (mchNo, cp, wp, mp, ap, st, l, ml, ms, md, type) {
	if (type == "single") {
		let data = [cp, wp, mp, ap, st, l, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaid = ?, wechatPaid = ?, manualPaid = ?, amountPaid = ?, startTime = ?, locked = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "vending") {
		let data = [cp, wp, mp, ap, md, ms, ml, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaid = ?, wechatPaid = ?, manualPaid = ?, amountPaid = ?, myDet = ?, mySoft = ?, myLb = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "doubleTop") {
		let data = [cp, wp, mp, ap, st, l, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaidTop = ?, wechatPaidTop = ?, manualPaidTop = ?, amountPaidTop = ?, startTimeTop = ?, lockedTop = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "doubleBot") {
		let data = [cp, wp, mp, ap, st, l, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaidBot = ?, wechatPaidBot = ?, manualPaidBot = ?, amountPaidBot = ?, startTimeBot = ?, lockedBot = ? WHERE MchCode = ?'; 
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	}
}

module.exports.updateMoney = function (mchNo, cp, wp, mp, ap, tc, tw, tm, tp, type) {
	if (type == "detCoin") {
		let data = [cp, tc, tp, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaid = ?, totalCoin = ?, totalPaid = ? WHERE MchCode = ?';
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "Coin") {
		let data = [cp, ap, mchNo]
		let sql = 'UPDATE dailyTotalValue SET coinPaid = ?, amountPaid = ? WHERE MchCode = ?';
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "detWechat") {
		let data = [wp, tw, tp, mchNo]
		let sql = 'UPDATE dailyTotalValue SET wechatPaid = ?, totalWechat = ? totalPaid = ? WHERE MchCode = ?';
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "Wechat") {
		let data = [wp, ap, mchNo]
		let sql = 'UPDATE dailyTotalValue SET wechatPaid = ?, amountPaid = ? WHERE MchCode = ?';
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "detManual") {
		let data = [mp, tm, mchNo]
		let sql = 'UPDATE dailyTotalValue SET manualPaid = ?, totalManual = ? WHERE MchCode = ?';
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	} else if (type == "Manual") {
		let data = [mp, mchNo]
		let sql = 'UPDATE dailyTotalValue SET manualPaid = ? WHERE MchCode = ?';
		db.run(sql, data, function(err) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		//console.log("data has been updated")
		});
	}
}

function querydata(mchNo, key, myVI) {
	let tmpkey = key+"_tmp"
	let sql = 'SELECT '+key+' '+tmpkey+' FROM dailyTotalValue WHERE MchCode = ?';
	return db.get(sql, [mchNo], function(err, row) {
		//console.log("the data is "+row[tmpkey])
		myVI[mchNo][key] = row[tmpkey]
		if (err) {
    		return console.error(err.message);
  		}
  		return row[tmpkey]  		
	})
}

var readRecords = function(callback){
    var db = new sqlite3.Database('./db/laundry.db', sqlite3.OPEN_READONLY);
    db.serialize(function() {
        db.all("SELECT * FROM dailyTotalValue", function(err, allRows) {
            if(err != null){
                console.log(err);
                callback(err);
            }
            console.log(util.inspect(allRows));
            callback(allRows);
            db.close();
        });
    });
}

