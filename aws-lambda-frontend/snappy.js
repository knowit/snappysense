// -*- fill-column: 100 -*-

// Application logic for the snappysense front-end

// Border around the canvas, px
const BORDER = 20

// Colors we pick from to plot multiple things together
const COLORS = ["black","red","fuchsia","green","yellow","blue","teal","aqua"]

// The biggest gap in time we allow between observations that are part of the same run
const DELTA = 2*60*60

// At setup time, populate the selectors with devices and factors
function setup() {
    // {"device":device-id, "location":location-id}
    fetch("/devices").
	then((response) => response.json()).
	then((devices) => {
	    let devs = the_devices()
	    for (let d of devices) {
		devs.add(new_option(d.device))
	    }
	    devs.disabled = false
	})

    // {"factor":factor-id, "description":text}
    fetch("/factors").
	then((response) => response.json()).
	then((factors) => {
	    let facts = the_factor()
	    for (let f of factors) {
		facts.add(new_option(f.factor))
	    }
	    facts.disabled = false
	})

    // {"location":location-id, "description":text}
    fetch("/locations").
	then((response) => response.json()).
	then((locations) => {
	    let locs = the_locations()
	    for (let f of locations) {
		locs.add(new_option(f.location))
	    }
	    locs.disabled = false
	})
}

// Selection logic:
//  - we want to compute a list of device names
//  - if any locations are selected, then we must obtain the devices at thos
//    locations, this entails an extra query
//  - if any devices are selected, they also go into the list
//  - once we have all data, we perform the plotting

function perform_query() {
    let loc_sel = the_locations()
    if (loc_sel.selectedIndex != -1) {
	// there are locations
	// we perform /devices_by_location?location=loc-id for all locations selected
	fetch_devices_from_locations_then_query_stage_2()
    } else {
	perform_query_stage_2([])
    }
}

function fetch_devices_from_locations_then_query_stage_2() {
    let loc_sel = the_locations()
    let opts = loc_sel.selectedOptions
    let remaining = opts.length
    let dev_ids = []
    for ( let i = 0 ; i < opts.length; i++ ) {
	// The response here is a list of device objects, for better or worse
	let loc_id = opts[i].value
	fetch(`/devices_by_location?location=${loc_id}`).
	    then((response) => response.json()).
	    then((data) => {
		for ( let d of data ) {
		    dev_ids.push(d.device)
		}
		remaining--
		if (remaining == 0) {
		    perform_query_stage_2(dev_ids)
		}
	    })
    }
}

function perform_query_stage_2(dev_ids) {
    let dev_sel = the_devices()
    let opts = dev_sel.selectedOptions
    for ( let i = 0; i < opts.length; i++ ) {
	dev_ids.push(opts[i].value)
    }
    if (dev_ids.length == 0) {
	return
    }
    // Remove duplicates: sort + compress runs
    dev_ids.sort((x, y) => x < y ? -1 : (x > y ? 1 : 0))
    let xs = dev_ids
    dev_ids = [xs[0]]
    for ( let i = 1; i < xs.length ; i++ ) {
	if (xs[i] != dev_ids[dev_ids.length-1]) {
	    dev_ids.push(xs[i])
	}
    }

    let fac_sel = the_factor()
    if (fac_sel.selectedIndex == -1) {
	return
    }
    let factor = fac_sel.options[fac_sel.selectedIndex].value
    if (factor == "") {
	return
    }

    fetch_and_plot_all(dev_ids, factor)
}

// For each selected device, fetch the observations for it, and when we have all of them, trigger
// the plotting - we can't plot incrementally since the scale depends on all of the data.
function fetch_and_plot_all(dev_ids, factor) {
    let num_left = dev_ids.length
    let accum = []
    for ( let dev_id of dev_ids ) {
	fetch(`/observations_by_device_and_factor?device=${dev_id}&factor=${factor}`).
	    then((response) => response.json()).
	    then((data) => {
		accum.push({"device":dev_id, "observations":data})
		num_left--
		if (num_left == 0) {
		    plot_data(factor, accum)
		}
	    })
    }
}

// datas ::= [{"device":device-id, "observations":[[sent, observation], ...]}, ...]
function plot_data(factor, datas) {
    // Filter the data: if the time is not credible (due to device not being properly configured
    // with the time) then discard the datum
    let cutoff = new Date(2023, 1, 1).valueOf()/1000
    for ( let d of datas ) {
	d.observations = d.observations.filter((x) => x[0] >= cutoff)
    }

    // Discard devices that have no observations left
    datas = datas.filter((x) => x.observations.length > 0)

    // Sort the observations per device ascending by sent timestamp
    for ( let d of datas ) {
	d.observations.sort((x, y) => x[0] - y[0])
    }

    // Sort the devices by name
    datas.sort((x, y) => x.device < y.device ? -1 : (x.device > y.device ? 1 : 0))

    // Compute min and max timestamp and min and max observation value across all the data sets.
    let min_time = +Infinity
    let max_time = -Infinity
    let min_obs = +Infinity
    let max_obs = -Infinity
    for ( let d of datas ) {
	min_time = Math.min(min_time, Math.min.apply(null, d.observations.map((x) => x[0])))
	max_time = Math.max(max_time, Math.max.apply(null, d.observations.map((x) => x[0])))
	min_obs = Math.min(min_obs, Math.min.apply(null, d.observations.map((x) => x[1])))
	max_obs = Math.max(max_obs, Math.max.apply(null, d.observations.map((x) => x[1])))
    }

    let cv = the_canvas()
    let cx = cv.getContext("2d")
    let blank = cx.createImageData(cv.width, cv.height)
    cx.putImageData(blank, 0, 0)

    let colornum = 0
    for ( let d of datas ) {
	let c = COLORS[colornum++ % COLORS.length]
	// Split the time series into separate stretches for which we have data
	let observations = d.observations
	let buckets = []
	let bucket = []
	for ( let obs of observations ) {
	    if (bucket.length == 0 || bucket[bucket.length-1][0] + DELTA > obs[0]) {
		bucket.push(obs)
	    } else {
		buckets.push(bucket)
		bucket = [obs]
	    }
	}
	if (bucket.length > 0) {
	    buckets.push(bucket)
	}
	for ( let b of buckets ) {
	    plot_one_device(cv, b, min_time, max_time, min_obs, max_obs, c)
	}
	cx.font = "16px serif"
	cx.fillStyle = c
	cx.fillText(d.device, cv.width - 200, colornum * 15)
	cx.fillStyle = "black"
    }

    // Y axis has observations.  We have room for about eight values.
    // TODO: Better labels
    cx.fillText(onedec(max_obs), 0, cv.height - (cv.height - BORDER))
    cx.fillText(onedec(min_obs), 0, cv.height - BORDER)

    // X axis has time.  Time labeling is sort of tricky.
    // TODO: Better labels

    // Label each end point first with mmm-dd
    cx.fillText(mmm_dd(min_time), BORDER, cv.height)
    cx.fillText(mmm_dd(max_time), cv.width - 2*BORDER, cv.height)
    
    // Three days is a short time
    const short_time = 3*24*60*60
    if (max_time - min_time <= short_time) {
	// Label every midnight as a long tick and every hour as a short tick
	// First tick is at ceil_hour(min_time)
	// Last tick is at ceil_hour(max_time)
	// If an hour is 0 then it's midnight
	
    } else {
	// Label every midnight as a tick along the bottom line
    }

//    let s = obs.join("\n")
//    the_debug_dump().innerText = dev_id + "\n" + factor + "\n\n" + s
}

function plot_one_device(cv, observations, min_time, max_time, min_obs, max_obs, color) {
    let cx = cv.getContext("2d")
    cx.strokeStyle = color
    cx.lineWidth = 2
    let width = cv.width - 2*BORDER
    let height = cv.height - 2*BORDER
    // Dang it, y axis has to be inverted...
    let p = cx.beginPath()
    let first = true
    for ( let [t,o] of observations ) {
	let x = BORDER + Math.round((t - min_time) / (max_time - min_time) * width)
	let y = BORDER + Math.round((o - min_obs) / (max_obs - min_obs) * height)
	if (first) {
	    cx.moveTo(x, cv.height - y)
	} else {
	    cx.lineTo(x, cv.height - y)
	}
	first = false
    }
    cx.stroke()
}
	
function mmm_dd(n) {
    let d = new Date(n*1000)
    let mmm = ["jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"]
    return mmm[d.getMonth()] + "-" + d.getDate()
}

function onedec(n) {
    return Math.round(n*10)/10
}

function the_devices() {
    return document.getElementById("select-device-id")
}

function the_locations() {
    return document.getElementById("select-location-id")
}

function the_factor() {
    return document.getElementById("select-factor")
}

function the_button() {
    return document.getElementById("query-btn")
}

function the_canvas() {
    return document.getElementById("plot")
}

function the_debug_dump() {
    return document.getElementById("xs")
}

function new_option(val) {
    let opt = document.createElement('option')
    opt.value = val
    opt.label = val
    return opt
}
