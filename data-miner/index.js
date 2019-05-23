const server = require("http").createServer(handler);
const io = require("socket.io")(server);
const fs = require("fs");
const path = require("path");
const config = require("./config");
let inMemoryDB = null;
const sessions = {
    
};

// db 

// users: user[]
// data: item[]

// db user

// login: string
// password: string

// db item

// id: string
// duration: string
// bytesCount: int
// isBisy: bool
// spectrogram: string
// waveGraph: string
// frames: itemFrame[]

// db itemFrame

// id: string
// assumptions: itemFrameAssumption[]

// db itemFrameAssumption

// login: string
// assumption: string

const handler = (request, response) => {
    console.log("Request starting...");

    let filePath = "." + request.url;
    if (filePath == "./")
        filePath = "./index.html";

    const extname = path.extname(filePath);
    const contentType = "text/html";
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

}

const ioHandler = (socket) => {
    socket.emit("news", { hello: "world" });
    socket.on("get_data_request", function (meassage) {
        onGetData(socket, message);
    });
    socket.on("set_data_request", function (data) {
        onSetData(socket, message);
    });

}

const onGetData = (socket, message) => {
    const request = JSON.parse(message);
    if (!inMemoryDB) {
        loadDB();
    }
    socket.emit(inMemoryDB);
}

const onSetData = (socket, message) => {
    const request = JSON.parse(message);

}

const loadDB = () => {
    const json = fs.readFileSync(config.dbFileName);
    inMemoryDB = JSON.parse(json);
}
const ifFilesChanged = () => {

}

server.listen(config.port, config.address);
console.log("Server running");
io.on("connection", ioHandler); 1
console.log("Web sockets server running");
console.log(`Browse data-miner http://${config.address}:${config.port}`)