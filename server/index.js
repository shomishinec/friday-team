const net = require('net');
const config = require("./config");
const resultSaver = require("./utils/result-saver")

const server = net.createServer((socket) => {
    let receiverBytes = 0;
    const lpcmBuffer = Buffer.alloc(32000 * 3);
    socket.on("error", function (err) {
        console.error("Error: " + err);
    });
    socket.on("data", (lpcmChunk) => {
        console.log("Received chunk");
        lpcmChunk.copy(lpcmBuffer, receiverBytes);
        receiverBytes += lpcmChunk.length;
        console.log("Total bytes received: " + receiverBytes);
        if (receiverBytes == 32000 * 3) {
            const fileName = (+ new Date()).toString();
            resultSaver.saveAudio(lpcmBuffer, fileName);
            resultSaver.saveAudioFrames(lpcmBuffer, fileName, 50);
            resultSaver.saveExcelPCMData(lpcmBuffer, fileName);
            resultSaver.saveSpectrogram(lpcmBuffer, fileName, 256);
            socket.end("Done");
            receiverBytes = 0;
        }
    });
});
server.listen(config.port, config.address);
console.log(`Server started at ${config.address}:${config.port}`);