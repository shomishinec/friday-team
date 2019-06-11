const http = require("http");
const io = require("socket.io");
const fs = require("fs");
const path = require("path");
const config = require("./server/config");
const types = require("./shared/types");
const classes = require("./shared/classes");
const db = require("./server/db");
const session = require("./server/session")

const handler = (request, response) => {
    console.log("Request starting...");

    let filePath = "." + request.url;
    if (filePath == "./")
        filePath = "./public/index.html";

    const extname = path.extname(filePath);
    let contentType = "text/html";
    switch (extname) {
        case ".js":
            contentType = "text/javascript";
            break;
        case ".css":
            contentType = "text/css";
            break;
        case ".json":
            contentType = "application/json";
            break;
        case ".png":
            contentType = "image/png";
            break;
        case ".jpg":
            contentType = "image/jpg";
            break;
        case ".wav":
            contentType = "audio/wav";
            break;
    }

    fs.readFile(filePath, function (error, content) {
        if (error) {
            if (error.code == "ENOENT") {
                fs.readFile("./404.html", function (error, content) {
                    response.writeHead(200, { "Content-Type": contentType });
                    response.end(content, "utf-8");
                });
            }
            else {
                response.writeHead(500);
                response.end("Sorry, check with the site admin for error: " + error.code + " ..\n");
                response.end();
            }
        }
        else {
            response.writeHead(200, { "Content-Type": contentType });
            response.end(content, "utf-8");
        }
    });

};

const ioHandler = (socket) => {
    // auth
    socket.on(types.loginRequest, function (dto) {
        const user = classes.User.fromDto(dto);
        const db = db.getDB();
        if (db.users.findIndex(dbUser => dbUser.login === user.login && dbUser.password === user.password) !== -1) {
            const sessionKey = session.start();
            socket.emit(types.loginResponse, JSON.stringify(new classes.Response(true, sessionKey)));
        } else {
            socket.emit(types.loginResponse, JSON.stringify(new classes.Response(false, null, "user not found")));
        }
    });
    // load data
    socket.on(types.loadDataRequest, function (dto) {
        socket.emit(types.loadDataResponse, JSON.stringify(db.getDB().data));
    });
};


const server = http.createServer(handler);
server.listen(config.port, config.address);
console.log("Server running");
io(server).on("connection", ioHandler);
console.log("Web sockets server running");
console.log(`Browse data-miner http://${config.address}:${config.port}`)