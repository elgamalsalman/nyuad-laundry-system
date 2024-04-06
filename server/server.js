import path from "path";
import http from "http";

import express from "express";
import { WebSocketServer } from "ws";
import nodemailer from "nodemailer";

// ---------- Global Constants ----------

// --- server data ---
const port = 8080;
const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });
const client_ping_delay = 5000;
const client_update_delay = 5000;

// --- server clients data ---
let clients = new Set();
let client_intervals = new Map();

// --- buildings data ---
const new_building = {
	washers_count: 0,
	dryers_count: 0,
};

// --- email management ---
const email_service = "gmail";
const email = "elgamalsalman@gmail.com";
const email_password = "APP_PASSWORD_HERE";

// ---------- Global Variables ----------

// --- buildlings data ---
let buildings = {};
for (let num = 1; num <= 6; num++) {
	if (num === 3 || num === 4) continue;
	for (let sub of ['a', 'b', 'c']) {
		buildings["a" + num + sub] = { ...new_building };
		buildings["a" + num + sub].machines = {};
		buildings["a" + num + sub].queues = {
			w: [],
			d: [],
		};
	}
}
for (let num = 3; num <= 4; num++) {
	buildings["a" + num] = { ...new_building };
	buildings["a" + num].machines = {};
	buildings["a" + num].queues = {
		w: [],
		d: [],
	};
}

// --- email management ---
let mailer = nodemailer.createTransport({
  service: email_service,
  auth: {
    user: email,
    pass: email_password,
  },
});


// ---------- Helper Functions ----------

const send_email = (net_id, subject, content) => {
	let mail_options = {
		from: email,
		to: `${net_id}@nyu.edu`,
		subject: subject,
		text: content,
	};

	mailer.sendMail(mail_options, function(error, info){
		if (error) {
			console.log(error);
		} else {
			console.log('email sent: ' + info.response);
		}
	});
};

const confirm_reserver = (net_id, building, machine_type_code) => {
	// tbc: send email
	console.log(`confirming reserver ${net_id} at ${building} of ${machine_type_code}`);

	let machine_type;
	if (machine_type_code === 'w') {
		machine_type = "Washing Machine";
	} else {
		machine_type = "Dryer";
	}

	send_email(
		net_id,
		`Confirmation of Your Place in the ${machine_type} Queue!`,
		`This is a confirmation email that you are now on the queue for a ${machine_type} ` +
		`at ${building}! Another email will be sent when a ${machine_type} is free, stay alert!`,
	);
};

const notify_reserver = (net_id, building, machine_id) => {
	// tbc: send email
	console.log(`sending reserver ${net_id} that ${building} ${machine_id} is free`);

	let machine_type;
	if (machine_id[0] === 'w') {
		machine_type = "Washing Machine";
	} else {
		machine_type = "Dryer";
	}

	send_email(
		net_id,
		`Your ${machine_type} is Ready!`,
		`Machine ${machine_id} in building ${building} is reserved right now for you to use! ` +
		`Please come use it quickly, it will be passed to the next reserver if not used for ` +
		`15 minutes!`,
	);
};

const confirm_follower = (net_id, building, machine_id) => {
	// tbc: send email
	console.log(`confirm follower ${net_id} of ${building} ${machine_id}`);

	let machine_type;
	if (machine_id[0] === 'w') {
		machine_type = "Washing Machine";
	} else {
		machine_type = "Dryer";
	}

	send_email(
		net_id,
		`Confirmation of Your Follow of ${building} machine ${machine_id}!`,
		`This is an email to confirm that you will be sent another email once the machine ` +
		`${machine_id} in building ${building} is done!`,
	);
};

const notify_follower = (net_id, building, machine_id) => {
	// tbc: send email
	console.log(`notifying ${net_id} that ${building} ${machine_id} is free`);

	let machine_type;
	if (machine_id[0] === 'w') {
		machine_type = "Washing Machine";
	} else {
		machine_type = "Dryer";
	}

	send_email(
		net_id,
		`Your ${machine_type} is Ready!`,
		`Machine ${machine_id} in building ${building} is done!`,
	);
};

const stream_data = () => {
	// setup new connections
	wss.on("connection", (ws, req) => {
		clients.add(ws);

		ws.isAlive = true;
		ws.on("pong", function() {
			this.isAlive = true;
		});

		ws.send(JSON.stringify({
			type: "buildings",
			data: buildings,
		}));
		const curr_interval = setInterval(() => {
			ws.send(JSON.stringify({
				type: "buildings",
				data: buildings,
			}));
		}, client_update_delay);
		client_intervals.set(ws, curr_interval);
  });

	// remove disconnected connections and stop
	// sending them data
	setInterval(() => {
		const clients_to_delete = [];
		clients.forEach((ws) => {
			if (ws.isAlive === false) {
				clearInterval(client_intervals.get(ws));
				clients_to_delete.push(ws);
			} else {
				ws.isAlive = false;
				ws.ping();
			}
		});

		for (const ws of clients_to_delete) {
			clients.delete(ws);
		}
	}, client_ping_delay);
};

// ---------- Route Management ----------
app.get("/CLIENT", (request, response, next) => {
	const { type, net_id, building, machine_type, machine_id } = request.query;

	// enqueue user
	if (type == "ENQUEUE") {
		console.log(`enqueue ${net_id} in ${building} ${machine_type}`);
		// add to queue
		buildings[building].queues[machine_type].push(net_id);
		confirm_reserver(net_id, building, machine_type);
		console.log(buildings[building].queues[machine_type]);
	} else if (type == "NOTIFY") {
		buildings[building].machines[machine_id].followers.push(net_id);
		confirm_follower(net_id, building, machine_id);
	}
});

app.get("/MONITOR", (request, response, next) => {
	const { building, machine_id, state } = request.query;
	console.log({ building, machine_id, state });

	const machine = buildings[building].machines[machine_id];
	if (machine === undefined) {
		buildings[building].machines[machine_id] = {
			state: "USED",
			followers: [],
		};
	}

	let response_data = JSON.stringify({
		state: "OK",
	});
	const prev_state = buildings[building].machines[machine_id].state;

	// if the machine became free
	if (state === "FREE" && prev_state != "FREE") {
		// check if there is someone in the queue
		let queue = buildings[building].queues[machine_id[0]];
		if (queue.length > 0) {
			// reserve waching machine for the person
			let net_id = queue[0];
			notify_reserver(net_id, building, machine_id);
			queue.shift();
			response_data = JSON.stringify({
				state: "RESERVED",
				reserver: net_id,
			});
			buildings[building].machines[machine_id].state = "RESERVED";
		} else {
			// free the washing machine
			if (machine_id[0] === "w") {
				buildings[building].washers_count++;
			} else {
				buildings[building].dryers_count++;
			}
			buildings[building].machines[machine_id].state = "FREE";

			// notify who needs to be notified
			for (let follower of buildings[building].machines[machine_id].followers) {
				notify_follower(follower, building, machine_id);
			}
			buildings[building].machines[machine_id].followers = [];
		}

	// if the machine became used
	} else if (state === "USED" && prev_state != "USED") {
		if (machine_id[0] === "w") {
			buildings[building].washers_count--;
		} else {
			buildings[building].dryers_count--;
		}
		buildings[building].machines[machine_id].state = "USED";
	} 

	response.send(response_data);
});

app.use("/", express.static(path.join(process.cwd(), "public")));

app.get("/", (request, response, next) => {
	response.sendFile(path.join(process.cwd(), "views", "main.html"));
});

server.listen(port, () => {
	console.log(`Listening on port ${port}`);
});

// ---------- Async Main Program ----------

(async () => {
	stream_data();
})();
