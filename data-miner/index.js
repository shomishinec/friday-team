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
        const user = parse(dto, classes.User).data;
        const users = db.getDB().users;
        if (users.findIndex(dbUser => dbUser.login === user.login && dbUser.password === user.password) === -1) {
            socket.emit(types.loginResponse, err("user not found"));
            return;
        }
        const sessionKey = session.start(user.login);
        socket.emit(types.loginResponse, ok(new classes.Auth(user.login, sessionKey)));
    });
    // load data
    socket.on(types.loadDataRequest, function (dto) {
        const auth = parse(dto).auth;
        if (!session.check(auth)) {
            socket.emit(types.loadDataResponse, err("user unauthorize"));
            return;
        }
        const dataItems = db.getDB().dataItems;
        socket.emit(types.loadDataResponse, ok(dataItems));
    });
    // set data
    socket.on(types.setDataRequest, function (dto) {
        const request = parse(dto);
        if (!session.check(request.auth)) {
            socket.emit(types.setDataResponse, err("user unauthorize"));
            return;
        }
        const changedDataItem = request.data;
        const db = db.getDB();
        const index = db.dataItems.findIndex(dataItem.id === changedDataItem.id);
        if (index === -1) {
            socket.emit(types.setDataResponse, err("data item not found"));
            return;
        }
        db.dataItems[index] = changedDataItem;
        db.setDB(db);
        socket.emit(types.setDataResponse, ok());
        socket.emit(types.dataHasModified, ok(changedDataItem));
    });
    // lock data
    socket.on(types.lockDataRequest, function (dto) {
        const request = parse(dto);
        if (!session.check(request.auth)) {
            socket.emit(types.lockDataResponse, err("user unauthorize"));
            return;
        }
        const dataItemId = request.id;
        const db = db.getDB();
        const index = db.dataItems.findIndex(dataItem.id === dataItemId);
        if (index === -1) {
            socket.emit(types.lockDataResponse, err("data item not found"));
            return;
        }
        if (db.dataItems[index].lockedBy) {
            socket.emit(types.lockDataResponse, err(`data locked by ${request.auth.login}`));
            return;
        }
        db.dataItems[index].locked = true;
        db.dataItems[index].lockedBy = request.auth.login;
        db.setDB(db)
        socket.emit(types.lockDataResponse, ok());
        socket.emit(types.dataLocked, ok(db.dataItems[index].id))
    });
    // free data
    socket.on(types.freeDataRequest, function (dto) {
        const request = parse(dto);
        if (!session.check(request.auth)) {
            socket.emit(types.freeDataResponse, err("user unauthorize"));
            return;
        }
        const dataItemId = request.id;
        const db = db.getDB();
        const index = db.dataItems.findIndex(dataItem.id === dataItemId);
        if (index === -1) {
            socket.emit(types.freeDataResponse, err("data item not found"));
            return;
        }
        if (db.dataItems[index].lockedBy !== request.auth.login) {
            socket.emit(types.freeDataResponse, err(`data locked by ${request.auth.login}`));
            return;
        }
        if (!db.dataItems[index].locked) {
            socket.emit(types.freeDataResponse, err("data item is free"));
            return;
        }
        db.dataItems[index].locked = false;
        db.dataItems[index].lockedBy = null
        db.setDB(db)
        socket.emit(types.freeDataResponse, ok(dataItems));
        socket.emit(types.dataFree, ok(db.dataItems[index].id))
    });
};

const parse = (dto, Class) => {
    return classes.Request.fromDto(dto, Class ? Class.fromDto : null);
}

const ok = (data) => {
    return JSON.stringify(new classes.Response(true, data));
};

const err = (error) => {
    return JSON.stringify(new classes.Response(false, null, error));
};

const server = http.createServer(handler);
server.listen(config.port, config.address);
console.log("Server running");
io(server).on("connection", ioHandler);
console.log("Web sockets server running");
console.log(`Browse data-miner http://${config.address}:${config.port}`)