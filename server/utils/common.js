const increaseFrequency = (buffer) => {
    const k = 2;
    const byterate = 2;
    const newBufferLength = buffer.length * k;
    const newBuffer = new Buffer.alloc(newBufferLength);
    for (let i = 0; i < buffer.length / byterate - 1; i += byterate) {
        const leftValue = readAudioBuffer(buffer, byterate, i);
        const rightValue = readAudioBuffer(buffer, byterate, i + 1);
        const signalDelta = rightValue - leftValue;
        const algleDelta = 1 / k;
        for (let j = 0; j <= k; j++) {
            let intermidateValue = leftValue + (signalDelta * (algleDelta * j));
            writeAudioBuffer(intermidateValue, newBuffer, byterate, (i * k) + j);
        }
    }
    return newBuffer;
}

const readAudioBuffer = (buffer, byterate, index) => {
    switch (byterate) {
        case 1:
            return buffer[index];
        case 2:
            return buffer.readUInt16LE(index * byterate);
        case 4:
            return buffer.readUInt32LE(index * byterate);
    }
}

function writeAudioBuffer(value, buffer, byterate, index) {
    switch (byterate) {
        case 1:
            return buffer[index] = value;
        case 2:
            return buffer.writeUInt16LE(value, index * byterate);
        case 4:
            return buffer.writeUInt32LE(value, index * byterate);
    }
}

const pad = (number, length) => {
    var string = "" + number;
    while (string.length < length) {
        string = "0" + string;
    }
    return string;
}

function logFile(buffer, byterate) {
    let result = "";
    for (i = 0; i < buffer.length / byterate; i += byterate) {
        result += utils.readAudioBuffer(buffer, byterate, i) + ", ";
    }
    console.log(result);
}

module.exports = {
    increaseFrequency,
    readAudioBuffer,
    writeAudioBuffer,
    pad,
    logFile
}