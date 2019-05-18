const fs = require("fs")
// const utils = require("./utils")

module.exports.save = (buffer) => {
    const fileName = (+ new Date());
    // buffer = utils.increaseFrequency(buffer);
    writeFile(fileName, buffer);
}


function writeFile(fileName, buffer, frequency = 16, bitrate = 16) {
    fileName = fileName + "." + frequency + "kHz." + bitrate + "bit.wav"
    outStream = fs.createWriteStream(fileName);

    var headerBuffer = new Buffer.alloc(1024);
    headerBuffer.write("RIFF", 0);
    /* file length */
    headerBuffer.writeUInt32LE(36 + buffer.length, 4);
    headerBuffer.write("WAVE", 8);
    headerBuffer.write("fmt ", 12);

    /* format chunk length */
    headerBuffer.writeUInt32LE(16, 16);

    /* sample format (raw) */
    headerBuffer.writeUInt16LE(1, 20);

    /* channel count */
    headerBuffer.writeUInt16LE(1, 22);

    /* sample rate */
    headerBuffer.writeUInt32LE(frequency * 1000, 24);

    /* byte rate */
    headerBuffer.writeUInt32LE(frequency * 1000 * bitrate / 8, 28);

    /* block align */
    headerBuffer.writeUInt16LE(1 * bitrate / 8, 32);

    /* bits per sample */
    headerBuffer.writeUInt16LE(bitrate, 34);

    /* data chunk identifier */
    headerBuffer.write("data", 36);

    /* data chunk length */
    headerBuffer.writeUInt32LE(0, 40);

    outStream.write(headerBuffer.slice(0, 44));
    outStream.write(buffer.slice(0, buffer.length));
    outStream.end();

    console.log(fileName + " writen, size = " + buffer.length + " bytes");
};