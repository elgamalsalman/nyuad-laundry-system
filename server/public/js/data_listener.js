const url = window.location;

const ws = new WebSocket(`ws://${url.host}/`);

// data
let buildings = {};

// getting building id
const get_building_id = () => {
	const building = document.querySelector("#building").value;
	const sub_building = document.querySelector("#sub-building").value;

	const building_id = (building + sub_building).toLowerCase();
	return building_id;
};

// getting net_id
const get_net_id = () => {
	const net_id = document.querySelector("#net-id").value;
	return net_id;
};

// on recieving new data
ws.onmessage = (event) => {
  const { type, data } = JSON.parse(event.data); 
	
	if (type === "buildings") {
		buildings = data;
		update_information();
	}
};

// updating information on new buildling selection
const update_information = () => {
	const building_id = get_building_id();
	const washers_number = document.querySelector("#washing-machines-progress .progress-number");
	washers_number.innerHTML = buildings[building_id].washers_count;
	const dryers_number = document.querySelector("#dryers-progress .progress-number");
	dryers_number.innerHTML = buildings[building_id].dryers_count;
};
const building_selection = document.querySelector("#building");
const sub_building_selection = document.querySelector("#sub-building");
building_selection.onchange = update_information;
sub_building_selection.onchange = update_information;

// joinig the queues
const create_enqueue_func = (machine_type) => {
	return () => {
		const building_id = get_building_id();
		const net_id = get_net_id();

		// only join if no empty machines
		if (
			(machine_type === 'w' && buildings[building_id].washers_count > 0) ||
			(machine_type === 'd' && buildings[building_id].dryers_count > 0)
		) {
			return;
		}

		console.log(`joining ${machine_type} queue`);
		
		// create fetch link
		let link = `http://${url.host}/CLIENT?`
		link += `type=ENQUEUE&net_id=${net_id}`;
		link += `&building=${building_id}`;
		link += `&machine_type=${machine_type}`;
		fetch(link);
	};
};
const washers_queue_btn = document.querySelector("#washers-queue-btn");
washers_queue_btn.onclick = create_enqueue_func("w");
const dryers_queue_btn = document.querySelector("#dryers-queue-btn");
dryers_queue_btn.onclick = create_enqueue_func("d");

// getting notified
const notify_btn = document.querySelector("#notify-btn");
notify_btn.onclick = () => {
	const machine_id = document.querySelector("#notify-machine").value.toLowerCase();

	const building_id = get_building_id();
	const net_id = get_net_id();
	
	let link = `http://${url.host}/CLIENT?`
	link += `type=NOTIFY&net_id=${net_id}`;
	link += `&building=${building_id}`;
	link += `&machine_id=${machine_id}`;
	fetch(link);
};
