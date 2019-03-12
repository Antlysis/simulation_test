/////////////////////////////////////
///// Required Packages /////////////
/////////////////////////////////////

var express = require('express');
var myParser = require("body-parser");
var app = express();
var server = require('http').Server(app);
var io = require('socket.io')(server);
var querystring = require('querystring');
var json2csvParser = require('json2csv').Parser;
var nodemailer = require('nodemailer');
var paperboy = require('./lib/paperboy');
var varItem = require('./credentials/variables.js');
var path = require('path');
var myWebroot = path.join(path.dirname(__filename), 'webroot');
var parseurl = require('parseurl');
var session = require('express-session');
var schedule = require('node-schedule');
var mustacheExpress = require('mustache-express');
var math = require('math');
var chkHeartbeat = require('./scripts/chkHB.js')
//var rpting = require('./scripts/reporting.js')
var mymqtt = require('./scripts/mqtt.js')
//var rmapi = require('./scripts/RMapi.js')
var moment = require('moment');
var fetch = require('node-fetch');
var childProcess = require('child_process');
var githubUsername = 'lskjames'

var transporter = nodemailer.createTransport({
	host: 'smtp.gmail.com',
    port: 465,
    secure: true, // use SSL
	auth:{
		user: 'ptutm.jameslsk@gmail.com',
		pass: 'james009'
	}
});
var doneSent = 0;
var counting = 0;
const sqlite3 = require('sqlite3').verbose();
const ePaymentCsv = "EpaymentReport";
const data = require('./auth/userData.js');
const data2 = require('./auth/data.js');
const outlet = require('./data/outlet.js');

var myTransRecord = {}
var tmp = {}
var pricing_data = {}

// const Gpio = require('onoff').Gpio;
// const jamming = new Gpio(4, 'in', 'rising');

// jamming.watch((err, value) => {
//      if (err) {
//              throw err;
//      }
//      console.log("it is jamming now")
//      counting++
//      if (counting == 5) {
//          if (!doneSent) {
//              var mailOptions = {
//                      from: 'ptutm.jameslsk@gmail.com',
//                      to: 'jamesleesukey@gmail.com',
//                      subject: 'Sending Email to notify that someone is jamming the network at' + outlet.name ,
//                      text: 'Please check !!!'
//              };
//              transporter.sendMail(mailOptions, function(error, info){
//                      if (error) {
//                              console.log(error);
//                      } else {
//                              console.log('Email sent: ' + info.response);
//                      }
//              });
//              doneSent = 1;
//              counting = 0;
//          }
//      } else if (counting == 1000) {
//              doneSent = 0;
//              counting = 0;
//      }
// })

app.engine('mustache', mustacheExpress());
app.set('views', './views');
app.set('view engine', 'mustache');
app.use(myParser.urlencoded({extended : false}));
app.use(myParser.json());
app.use(session({
	secret: 'keyboard cat',
	resave: false,
	saveUninitialized: true
}));

app.use(function (req, res, next) {
	var views = req.session.views;

	if (!views) {
		views = req.session.views = {};
	}

	//get the url pathname
	var pathname = parseurl(req).pathname;

	// count the views
	views[pathname] = (views[pathname] || 0) + 1
	next();
})


function authenticate_admin(req, res, username, password) {
	let db = new sqlite3.Database('./mydb/laundry.db', sqlite3.OPEN_READWRITE, (err) => {
	  	if (err) {
	    	console.error(err.message);
	  	}
  		console.log('Connected to the laundry database.');
	});
	var userfound = false;
	db.serialize(function() {
		let sql = 'SELECT username us FROM admin';
		db.all(sql, [], function(err, row) {
			console.log(row.length)
			const length = row.length;
			for(var i = 0; i < length; i++){
				console.log(row[i])
				if(row[i].us == username) {
					userfound = true;
					console.log("user found")
				} else {
					console.log("user not matched")
				}
			}
			if (userfound) {
				let sql2 = 'SELECT password pw, role r FROM admin WHERE username = ?';
				db.get(sql2, [username], function(err, row) {
					if (password == row.pw) {
						req.session.authenticated = true;
						req.session.username = username;
						req.session.role = row.r;
						console.log('User & Password Authenticated '+ req.session.role);
						console.log(req.session)
						//return req.session;
						//db.close();
					} else {
						req.session.authenticated = false;
					}
					if (req.session && req.session.authenticated){
						console.log("testing")
						if (req.session.role == "developer") {
							//console.debug("its developer")
							res.render('updateFw', { url: outlet.url, users: data2.users, brand: outlet.brand});
						} else if (req.session.role == "admin") {
							var tmpdata = []
							Object.keys(varItem).forEach(function(k){
								tmpdata.push(varItem[k])
							})
							console.log("its admin")
							res.render('admin_execution',{ url: outlet.url, machines: tmpdata, brand: outlet.brand, outlet: outlet.name});
						}
					} else {
						res.render('login_admin', {brand: outlet.brand, message: "Login Failed, please try again."})
					}
				})
			} else {
				req.session.authenticated = false;
				res.render('login_admin', {brand: outlet.brand, message: "User not found !"});
			}	
		})
			
		//console.log(req.session);
	})

	//
	
}

function authenticate_user(req, res, username, password) {
	let db = new sqlite3.Database('./mydb/laundry.db', sqlite3.OPEN_READWRITE, (err) => {
	  	if (err) {
	    	console.error(err.message);
	  	}
  		console.log('Connected to the laundry database.');
	});
	var userfound = false;
	db.serialize(function() {
		let sql = 'SELECT username us FROM users';
		db.all(sql, [], function(err, row) {
			console.log(row.length)
			const length = row.length;
			for(var i = 0; i < length; i++){
				console.log(row[i])
				if(row[i].us == username) {
					userfound = true;
					console.log("user found")
				} else {
					console.log("user not matched")
				}
			}
			if (userfound) {
				let sql = 'SELECT password pw, status act FROM users WHERE username = ?';
				db.get(sql, [username], function(err, row) {
					if (password == row.pw) {
						req.session.authenticated = true;
						req.session.username = username;
						req.session.actv = row.act;
						console.log('User & Password Authenticated');
						db.close();
					} else {
						req.session.authenticated = false;
					}
					if (req.session && req.session.authenticated) {
						if (req.session.actv == "Active") {
							var tmpdata = []
							Object.keys(varItem).forEach(function(k){
								tmpdata.push(varItem[k])
							})
							res.render('execution',{ url: outlet.url, machines: tmpdata, brand: outlet.brand, outlet: outlet.name});
						} else if (req.session.actv == "Inactive") {
							res.render('login_user', {brand: outlet.brand, message: "Please consult your manager for Account Activation."})
						}
					} else {
						res.render('login_user', {brand: outlet.brand, message: "Login Failed, please try again."})
					}
				})
			} else {
				req.session.authenticated = false;
				res.render('login_user', {brand: outlet.brand, message: "User not found !"})
			}
		})
				
	})
}

app.use((req, res, next) => {
	res.setHeader('Access-Control-Allow-Origin', '*');
	res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, PUT, PATCH, DELETE');
	res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type');
	res.setHeader('Access-Control-Allow-Credentials', true);
	next();
});

app.use(express['static']('public'));

///////////////////////////////////////////
////// INIT AND START MQTT CONNECTION /////
///////////////////////////////////////////
io.on('connection', function (socket) { // Notify for a new connection and pass the socket as parameter.
    console.log('new connection');
    ///socket.emit("devices")
    var incremental = 0;
    
    var update = setInterval(function () {
     	var tmpdata = []
		Object.keys(varItem).forEach(function(k){
			if (varItem[k].typeOfMachine.match("dex_dryer_double")) {
				if (!tmpdouble_top) {
					var tmpdouble_top = {}
				}
				if (!tmpdouble_bot) {
					var tmpdouble_bot = {}
				}
				if (varItem[k].active == true) {
					tmpdouble_top.active = "Online"
					tmpdouble_bot.active = "Online"
				} else if (varItem[k].active == false) {
					tmpdouble_bot.active = "Offline"
					tmpdouble_top.active = "Offline"
				}
				tmpdouble_top.version = varItem[k].version
				tmpdouble_bot.version = varItem[k].version
				if (varItem[k].locked.upper == true) {
					tmpdouble_top.locked = "Running";
					if (!tmpdouble_top.machineName) {
						var pattern = /[A-Z]+\-D[0-9]+/g
						var top = varItem[k].machineName.match(pattern)
						var topname = top.toString()
						tmpdouble_top.machineName = topname
					}
				} else {
					tmpdouble_top.locked = "Standby";
					if (!tmpdouble_top.machineName) {
						var pattern = /[A-Z]+\-D[0-9]+/g
						var top = varItem[k].machineName.match(pattern)
						var topname = top.toString()
						tmpdouble_top.machineName = topname
					}
				}
				if (varItem[k].locked.lower == true) {
					tmpdouble_bot.locked = "Running";
					if (!tmpdouble_bot.machineName) {
						var pattern2 = /[0-9]+$/g
						var bot  = varItem[k].machineName.match(pattern2)
						var botname = outlet.name + "-D" + bot
						tmpdouble_bot.machineName = botname
					}				
				} else {
					tmpdouble_bot.locked = "Standby";
					if (!tmpdouble_bot.machineName) {
						var pattern2 = /[0-9]+$/g
						var bot  = varItem[k].machineName.match(pattern2)
						var botname = outlet.name + "-D" + bot
						tmpdouble_bot.machineName = botname
					}
				}
				tmpdata.push(tmpdouble_top)
				tmpdata.push(tmpdouble_bot)
			} else {
				var tmpMachine = {}
				tmpMachine = Object.assign({}, varItem[k])
				if (varItem[k].active == true) {
					tmpMachine.active = "Online"
				} else if (varItem[k].active == false) {
					tmpMachine.active = "Offline"
				}
				if (varItem[k].locked == true) {
					tmpMachine.locked = "Running"
				} else if (varItem[k].locked == false) {
					tmpMachine.locked = "Standby"
				}
				if (varItem[k].typeOfMachine == "detergent") {
					tmpMachine.locked = "Standby"
				}
				tmpdata.push(tmpMachine)
			}
		})
  //       //console.log('emit new value', tmpdata);
        socket.emit("devices", tmpdata); // Emit on the opened socket.
     }, 2000);

    socket.on('disconnect', function() {
      console.log('Got disconnect!');
      clearInterval(update)
    })
});
refreshValue(varItem, mymqtt.mqttCN, mymqtt.mqttSUB)

/////////////////////////////////////
////CHECK HEARTBEAT /////////////////
/////////////////////////////////////

/////////// update all the daily recorded data ////////

setInterval(chkHeartbeat.checkHeartbeat, 60000, varItem);

/////////////////////////////////////
////// FUNCTION DECLARATION /////////
/////////////////////////////////////


function object_value (object) {
	var val = [];
	Object.keys(object).forEach(function(k) {
		val.push(object[k]);
	})
	return val;
}

/// initialize value from the database 

function refreshValue (VI, callback1, callback2) {
	let db = new sqlite3.Database('./mydb/laundry.db', sqlite3.OPEN_READWRITE, (err) => {
  	if (err) {
    	console.error(err.message);
  	}
  	console.log('Connected to the laundry database.');
	});
	db.serialize(function() {
		Object.keys(VI).forEach(function(k) {
			if (VI[k].typeOfMachine.match(/dex_dryer_double/g)) {
				let sql = 'SELECT totalTimeTop ttt, totalTimeBot ttb, totalRun tr, startTimeTop stt, startTimeBot stb, lockedTop lt, lockedBot lb, coinPaidTop cpt, coinPaidBot cpb, wechatPaidTop wpt, wechatPaidBot wpb, manualPaidTop mpt, manualPaidBot mpb, amountPaidTop apt, amountPaidBot apb FROM dailyTotalValue WHERE MchCode = ?';
				db.get(sql, [k], function(err, row) {
					if (err) {
	    				return console.error(err.message);
	  				}
	  				//console.log(VI[k].totalTime.upper) 
	  				VI[k].totalTime.upper = row.ttt
					VI[k].totalTime.lower = row.ttb
					VI[k].totalRun = row.tr
					VI[k].startTime.upper = row.stt
					VI[k].startTime.lower = row.stb
					if (row.lt == 0) {
						VI[k].locked.upper = false;
						console.log(k+" top is not running previously")
					} else if (row.lt == 1) {
						VI[k].locked.upper = true;
						console.log(k+" top is running previously")
					}
					if (row.lb == 0) {
						VI[k].locked.lower = false;
						console.log(k+" bot is not running previously")
					} else if (row.lb == 1) {
						VI[k].locked.lower = true;
						console.log(k+" bot is running previously")
					}
					VI[k].coinPaidTop = row.cpt; VI[k].coinPaidBot = row.cpb; 
					VI[k].wechatPaidTop = row.wpt; VI[k].wechatPaidBot = row.wpb;
					VI[k].manualPaidTop = row.wpt; VI[k].manualPaidBot = row.mpb;
					VI[k].amountPaidTop = row.wpt; VI[k].amountPaidBot = row.apb;
				})
			} else if (VI[k].typeOfMachine == "detergent") {
				let sql = 'SELECT totalRun tr, detergent d, softnr s, beg b, myDet md, mySoft ms, myLb ml FROM dailyTotalValue WHERE MchCode = ?';
				db.get(sql, [k], function(err, row) {
					if (err) {
	    				return console.error(err.message);
	  				}
	  				//console.log(VI[k].totalTime.upper) 
	  				VI[k].detergent = row.d
					VI[k].softnr = row.s
					VI[k].beg = row.b
					VI[k].myDet = row.md
					VI[k].mySoft = row.ms
					VI[k].myLb = row.ml
					VI[k].totalRun = row.tr
					//console.log(VI[k].totalTime.upper)
				})
			} else if (VI[k].typeOfMachine.match(/Washer/g)){
				let sql = 'SELECT totalTime tt, totalRun tr, totalColdRun tcr, totalWarmRun twr, totalHotRun thr, totalOtherRun tor, startTime st, locked lock FROM dailyTotalValue WHERE MchCode = ?';
				db.get(sql, [k], function(err, row) {
					if (err) {
	    				return console.error(err.message);
	  				}
	  				//console.log(VI[k].coldRun) 
	  				VI[k].totalTime = row.tt
					VI[k].totalRun = row.tr; VI[k].coldRun = row.tcr; VI[k].warmRun = row.twr; VI[k].hotRun = row.thr; VI[k].otherRun = row.tor;
					VI[k].startTime = row.st
					if (row.lock == 0) {
						VI[k].locked = false;
						console.log(k+" is not running previously")
					} else if (row.lock == 1) {
						VI[k].locked = true;
						console.log(k+" is running previously")
					}
					//console.log(VI[k].coldRun)
				})
			} else if (VI[k].typeOfMachine.match(/dryer/g)){
				let sql = 'SELECT totalTime tt, totalRun tr, startTime st, locked lock FROM dailyTotalValue WHERE MchCode = ?';
				db.get(sql, [k], function(err, row) {
					if (err) {
	    				return console.error(err.message);
	  				}
	  				//console.log(VI[k].totalTime)
	  				VI[k].totalTime = row.tt
	  				VI[k].totalRun = row.tr
	  				VI[k].startTime = row.st
	  				if (row.lock == 0) {
						VI[k].locked = false;
						console.log(k+" is not running previously")
					} else if (row.lock == 1) {
						VI[k].locked = true;
						console.log(k+" is running previously")
					}
	  				//console.log(VI[k].totalTime)
	  			})
	  		}
			let sql = 'SELECT totalPaid tp, totalCoin tc, totalWechat tw, wechatPaid wp, coinPaid cp, amountPaid ap, manualPaid mp, version v, totalManual tm FROM dailyTotalValue WHERE MchCode = ?';
			db.get(sql, [k], function(err, row) {
				if (err) {
	    			return console.error(err.message);
	  			}
	  			//console.log(VI[k].totalCoin)
  				VI[k].amountPaid = row.ap
  				VI[k].coinPaid = row.cp
  				VI[k].wechatPaid = row.wp
  				VI[k].manualPaid = row.mp
	  			VI[k].totalManual = row.tm
	  			VI[k].totalPaid = row.tp
				VI[k].totalCoin = row.tc
				VI[k].totalWechat = row.tw
				VI[k].version = row.v
				//console.log(VI[k].totalCoin)  		
			})
		})
		let sql = 'SELECT * FROM pricing WHERE outlet = ?';
		db.all(sql, [outlet.name], function(err, row) {
			if (err) {
	    		return console.error(err.message);
	  		}
	  		pricing_data = Object.assign({}, row[0]);
	  		console.log(pricing_data)
		})
		//console.log(outlet)
		db.close();
	})
	callback1();
	callback2(VI, pricing_data);
}

// rendering function for epayment 

//rmapi.queryQRcodes()

// function render (item, mchNo, res, mTR, VI, transId) {
// 	var timeNow = moment().format("DD/MM/YYYY HH:mm:ss")
// 	console.log(item)
// 	if (item.status == "SUCCESS") {
// 		console.log("check")
//         var amountToPay = item.order.amount/100
//         var status = item.status
//         var remark = "NA"
//         if (amountToPay <= 25) {
//             if (VI[mchNo].active) {
//             	console.log("thanks")
//                 res.render('response', {text: "Thanks for using our service, please come again next time."});
//                 mymqtt.onMachine(mchNo,amountToPay)
//                 if (VI[mchNo].typeOfMachine == "detergent") {
//                     VI[mchNo].totalWechat = VI[mchNo].totalWechat + amountToPay
//                     VI[mchNo].totalPaid = VI[mchNo].totalPaid + amountToPay
//                     VI[mchNo].wechatPaid = VI[mchNo].wechatPaid + amountToPay
//                     VI[mchNo].amountPaid = VI[mchNo].amountPaid, + amountToPay
//                     //(mchNo, cp, wp, mp, ap, tc, tw, tm, tp, type)
//                     mymqtt.updateMoney(mchNo, 0, VI[mchNo].wechatPaid, 0, 0, 0, VI[mchNo].totalWechat, 0, VI[mchNo].totalPaid, "detWechat")
//                     mymqtt.updateAccumulative(mchNo, "Wechat_received", amountToPay)
//                 } else {
//                     VI[mchNo].wechatPaid = VI[mchNo].wechatPaid + amountToPay 
//                     VI[mchNo].amountPaid = VI[mchNo].amountPaid + amountToPay
//                     mymqtt.updateMoney(mchNo, 0, VI[mchNo].wechatPaid, 0, VI[mchNo].amountPaid, 0, 0, 0, 0, "Wechat")
//                 }
//             } else {
//                 res.render('response', {text: "Sorry, This machine is not ready for Epayment right now. Please try again later. "});
//                 console.log(transId)
//                 status = "FULL_REFUNDED"
//                 remark = "Machine is not ready for epayment."
//                 rmapi.refundPayment(transId, item.order.amount, "The machine is not ready for Epayment right now.", "FULL")
//             }
//         } else {
// 			res.render('response', {text:"The payment is too much"});
// 			status = "FAILED"
// 			remark = "The payment exceeded RM25";
// 			rmapi.refundPayment(transId, item.order.amount, "The payment might be too much, please redo your payment, Thanks.", "FULL")
// 		}
// 		rpting.createEntry(mchNo, transId, item.order.title, amountToPay, item.payee.userId, timeNow, status, remark, mTR)
// 		rpting.save2csv("ePayment",mTR[transId], rpting.uploadNothing)
// 	} else {
// 		res.render('response', {text:"Thanks"});
// 	}
// }


//rmapi.queryTrans("181114094527020013647579", render, res, varItem, myTransRecord)
//rmapi.refundPayment("181129092243020016602292", 100, "i just wanna refund", "FULL")
//rmapi.refundPayment("181128120537020014768951", 100, "i just wanna refund", "FULL")
//rmapi.refundPayment("181128120626020019928039", 100, "i just wanna refund", "FULL")
//rmapi.refundPayment("181128120626020019928039", 100, "i just wanna refund", "FULL")
//rmapi.refundPayment("181122104612020011063619", 200, "i just wanna refund", "FULL")

//rmapi.refundPayment("181118035946020019303470", 100, "i just wanna refund", "FULL")
//rmapi.refundPayment("181118040745020011695718", 200, "i just wanna refund", "FULL")
//rmapi.refundPayment("181114170041020011629414", 400, "i just wanna refund", "FULL")
//queryStore()
//////////////////////////////
///////// API ROUTE //////////
//////////////////////////////

//app.listen(80);
server.listen(80);
console.log('App Server running at port 80');
