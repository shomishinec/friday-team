const net = require('net');
var server = net.createServer((socket)=>{
    let rawResponse;
    socket.on('data', function(chunk) {
        console.log(chunk);
        socket.write(chunk);
    });

});
//server.listen(1610, "127.0.0.1");
server.listen(8086, "172.20.10.3");
process.on('uncaughtException', function (err) {
    console.log(err);
});
