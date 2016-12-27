// Connec_backend polling helpers

poll = true;

function cbSetPoll(poll_on) {
  poll = poll_on;
  console.log("Polling is now ", poll);
  if (poll)
    cbPoll();
}

// authenticate
function cbAuthenticate(login, password) {
    //var login = 'sample';
    //var password = 'abcd';
  localStorage.setItem('cb_username', login);
  localStorage.setItem('cb_hostname', login.match(/^(.+)@(.+)$/)[2]);

  var request = cbConnection('POST', 'auth', false);
  request['data'] = JSON.stringify({ 'message': {'username': login, 'password': password, 'use_cors': true} });
  
  $.ajax(request).then(
    function(event) {
      // Success
      localStorage.setItem('authtoken', event.result.authtoken);
      localStorage.setItem('sessiontimeout', event.result["session-timeout"]);
      //cbPollSequence;
      window.location="cb_client.html";
    },
    function(event) {
      // error
      //location = '/missingcertificate';
      alert("Authentication failed");
    }
  );
}

// initial ping
function cbInitialPing() {
  $.ajax(cbConnection('POST', 'ping')).then(
    function(event) {
      // success
      cbPollSequence();
    },
    function(event) {
      // error
      //cbAuthenticate();
    }
  );
}

function cbPollSequence() {
  cbRoster();
  //cbSubscribeAll();
  cbStartPingTimer();
  cbPoll();
}

function cbPoll() {
  $.ajax(cbConnection('GET', 'poll')).then(
    function(event) {
      // success
      cbRenderEvent(event);
      updateRoster(event);
      if (poll)
        cbPoll();
    },
    function(event) {
      // error
      cbRenderEvent(event);
      cbInitialPing();
    }
  );
}

/*function cbSubscribeAll() {
  AGENTS_SUBSCRIBED = true;
  for (var i = AGENTS.length - 1; i >= 0; i--) {
    cbSubscribe(AGENTS[i]['uuid']);
  };
}*/

function cbSubscribeAll() {
  console.log("roster", roster);
  if (roster == [])
    return;
  group_count = roster["group"].length;
  for (var i = 0; i < group_count; i++) {
    group = roster["group"][i];
    member_count = group["members"].length;
    for (var j = 0; j < member_count; j++) {
      member = group["members"][j];
      device_count = member["device"].length;
      for (var k = 0; k < device_count; k++) {
	device = member["device"][k];
        cbSubscribe(device["uuid"]);
      }
    }
  };
}

function cbSubscribe(uuid) {
  $.ajax(cbConnection('POST', 'subscribe/' + uuid));
}

function cbUnSubscribe(uuid) {
  // DELETE method via CORS is not currently supported by backend
  $.ajax(cbConnection('DELETE', 'subscribe/' + uuid));
}

function cbStartPingTimer() {
  var timeout = ( localStorage.getItem('sessiontimeout') - 10 ) * 1000;
  var _this = this;
  setTimeout(function() { _this.cbTimeoutPing(); }, timeout);
}

function cbTimeoutPing() {
  console.log("Timeout ping");
  $.ajax(cbConnection('POST', 'ping')).then(
    function(event) {
      // success
      console.log("Timeout ping successful");
      cbStartPingTimer();
    },
    function(event) {
      // error
      cbAuthenticate();
    }
  );
}

function cbConnection (method, path, includeAuthtoken, keys) {
  includeAuthtoken = typeof includeAuthtoken !== 'undefined' ? includeAuthtoken : true;
  var authtokenString = '';
  if(includeAuthtoken) {
    authtokenString = '?authtoken=' + localStorage.getItem('authtoken');
  }
  if (keys) {
    if (includeAuthtoken)
      key_str = '&' + keys;
    else
      key_str = '?' + keys;
  }
  else
    key_str = '';

  return {
    url: 'https://' + localStorage.getItem('cb_hostname') + ':9091/' + path + authtokenString + key_str,
    contentType: 'text/plain',
    dataType: 'json',
    type: method,
    cors: true,
    async: true,
    processData: false
  };
}

function cbTransferAssisted(uuid, exten) {
  $.ajax(cbConnection('PUT', 'call/transfer/assisted/' + uuid + '/' + exten));
}

function cbTransferBlind(uuid, exten) {
  $.ajax(cbConnection('PUT', 'call/transfer/blind/' + uuid + '/' + exten));
}

function cbPark(uuid) {
  $.ajax(cbConnection('PUT', 'call/park/' + uuid));
}

function cbUnpark(uuid) {
  $.ajax(cbConnection('GET', 'call/park/' + uuid));
}

function cbOriginate(caller, callee) {
  $.ajax(cbConnection('PUT', 'call/originate/' + caller + '/' + callee));
}

function cbRenderEvent(event) {
  console.log("event", event);
}

function cbRoster() {
  $.ajax(cbConnection('GET', 'roster')).then(
    function(event) {
      // success
      cbRenderEvent(event);
      roster = event["result"];
      renderRoster();
      cbSubscribeAll();
      updateParkerList();
    },
    function(event) {
      // error
      cbRenderEvent(event);
    }
  );
}

function cbSearch(key, callback) {
  $.ajax(cbConnection('GET', 'roster/search', true, 'key=' + key)).then(
    function(event) {
      // success
      cbRenderEvent(event);
      if (typeof callback === "function")
	callback(event["result"]["search.roster"]);
    },
    function(event) {
      // error
      cbRenderEvent(event);
    }
  );
}

function statusText(status) {
  switch (status) {
  case "user_state_down":
  case "landline_state_down":
  case "cellphone_state_down":
    return "down";
  case "user_state_ring":
  case "landline_state_ring":
  case "cellphone_state_ring":
  case "user_state_ringing":
  case "landline_state_ringing":
  case "cellphone_state_ringing":
    return "ring";
  case "user_state_busy":
  case "landline_state_busy":
  case "cellphone_state_busy":
    return "busy";
  case "user_state_online":
  case "landline_state_online":
  case "cellphone_state_online":
    return "online";
  case "user_state_offline":
  case "landline_state_offline":
  case "cellphone_state_offline":
    return "offline";
  case "user_state_unknown":
  case "landline_state_unknown":
  case "cellphone_state_unknown":
    return "unknown";
  default:
    return status;
  }
}
