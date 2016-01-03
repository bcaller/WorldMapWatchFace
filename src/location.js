var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 1000 * 60 * 60, //1 hour
  timeout: 10000
};

function randomly() {
    locationSuccess({coords: {
        latitude: Math.random() * 180 - 90,
        longitude: Math.random() * 360 - 180
    }});
}

function locationSuccess(pos) {
    console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
    console.log('location found');
    sendLocation({
        latitude: Math.round(-(pos.coords.latitude - 90)*2/3 + 16),
        longitude: Math.round((147/180 * pos.coords.longitude)+215)
    });
}

function sendLocation(latlng){
    console.log('sending location ' + JSON.stringify(latlng));
    Pebble.sendAppMessage(latlng);
}

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
  }
);

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

Pebble.addEventListener('ready', 
  function(e) {
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    //setInterval(randomly, 5000); //Change location randomly every 5 seconds
  }
);


