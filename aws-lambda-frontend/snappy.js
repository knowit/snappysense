// -*- fill-column: 100 -*-

// Application logic for the snappysense front-end

// Border around the canvas, px
const BORDER = 20

// Colors we pick from to plot multiple things together
const COLORS = ["black","red","fuchsia","green","yellow","blue","teal","aqua"]

// If true, bucket observations together s.t. there are no gaps bigger than DELTA.
const DO_BUCKET = true

// The biggest gap in time we allow between observations that are part of the same bucket
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
    let launder_cutoff = new Date(2023, 1, 1).valueOf()/1000
    for ( let d of datas ) {
	d.observations = d.observations.filter((x) => x[0] >= launder_cutoff)
    }

    // Compute the cutoff for data we're interested in
    let time_sel = the_time_range()
    let time_range = time_sel.options[time_sel.selectedIndex].value
    let time_cutoff = 0
    switch (time_range) {
    case "last-day":
	time_cutoff = Date.now() - 24*60*60*1000
	break
    case "last-two-days":
	time_cutoff = Date.now() - 2*24*60*60*1000
	break
    case "last-week":
	time_cutoff = Date.now() - 7*24*60*60*1000
	break
    case "last-month":
	time_cutoff = Date.now() - 31*24*60*60*1000  // FIXME - maybe
	break
    case "last-year":
	time_cutoff = Date.now() - 365*24*60*60*1000 // FIXME - maybe
	break
    default:
	break
    }
    time_cutoff /= 1000

    // Sort the observations per device ascending by sent timestamp, and filter by cutoff
    for ( let d of datas ) {
	d.observations.sort((x, y) => x[0] - y[0])
	let i = 0
	while (i < d.observations.length && d.observations[i][0] < time_cutoff) {
	    i++
	}
	d.observations = d.observations.slice(i)
    }

    // Discard devices that have no observations left
    datas = datas.filter((x) => x.observations.length > 0)

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
    let date_ranges = []
    for ( let d of datas ) {
	let c = COLORS[colornum++ % COLORS.length]
	// Split the time series into separate stretches for which we have data
	let observations = d.observations
	let buckets = []
	if (DO_BUCKET) {
	    // TODO: Singleton buckets could be discarded, because they don't generally show up on
	    // the plot.  But in this case we should probably recompute max and min; and/or the
	    // bucketing should be done before computing max/min in the first place
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
	} else {
	    buckets.push(observations)
	}
	for ( let b of buckets ) {
	    date_ranges.push([b[0][0], b[b.length-1][0]])
	    plot_one_device(cv, b, min_time, max_time, min_obs, max_obs, c)
	}
	cx.font = "16px serif"
	cx.fillStyle = c
	cx.fillText(d.device, cv.width - 200, colornum * 15)
	cx.fillStyle = "black"
    }
    // TODO: Merge the date ranges
    // TODO: Somehow make the ranges selectable

    // Y axis has observations.  We have room for about eight values.
    let delta = (max_obs - min_obs) / 7
    for ( let i=0 ; i < 8; i++ ) {
	cx.fillText(onedec(min_obs + delta*i), 0, cv.height - BORDER - ((cv.height - 2*BORDER) / 7 * i))
    }

    // X axis has time.  We have room for about 16-32 values depending on how we label.
    // Time labeling is sort of tricky.
    //
    // To simplify, use time_range gotten above:
    // - for last-day, label every hour with hh only (24 labels)
    // - for last-two-days, label the even hours with hh (24 labels)
    // - for last-week, label 00 08 16 for all days (21 labels)
    // - for last-month, label midnight of every day (31 labels)
    // - for last-year, label 01 and 15 of every month (24 labels)
    // - for forever, label only left and right

    // Label each end point first with mmm-dd
    let numlabels = 0
    let labeler = null
    /*
    switch (time_range) {
    case "last-day": {
    // Note the labeling is not uniform, and it's tricky: if we only have data
    // for a few hours, they will fill the plot horizontally, and in that case,
    // there will not be 24 points.  This is true also if eg last-two-days were
    // selected.  Maybe what we have here is hour(max_time) - hour(min_time) points,
    // on the hour.
	let h_now = Date.now().getUTCHours()
	numlabels = 24
	labeler = function (l) {
	    // I need to know what the first hour is: it is the one following h_now
	    ...
	}
	break
    }
    }
    */
    if (numlabels == 0) {
	cx.fillText(mmm_dd(min_time), BORDER, cv.height)
	cx.fillText(mmm_dd(max_time), cv.width - 2*BORDER, cv.height)
    }
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

function the_time_range() {
    return document.getElementById("select-dates")
}

function the_button() {
    return document.getElementById("query-btn")
}

function the_canvas() {
    return document.getElementById("plot")
}

function new_option(val) {
    let opt = document.createElement('option')
    opt.value = val
    opt.label = val
    return opt
}
