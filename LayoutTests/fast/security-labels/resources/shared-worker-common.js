onconnect = function(event) {
    event.ports[0].onmessage = function(evt) { handleMessage(evt, event.ports[0]); };
};

function handleMessage(event, port) {
    if (event.data == "blocked")
        port.postMessage("PASS: blocked labeled data");
    else if (event.data == "ping")
        port.postMessage("PASS: received ping");
    else if (event.data == "done")
        port.postMessage("DONE");
    else
        port.postMessage("FAILURE: Received unknown message: " + event.data);
}
