module.exports.genereteResponceBuffer = (isSuccsess, comandString, value = null) => {
    let buf = Buffer.alloc(256);
    const comandNumber = ["godMode", "startTimer", "storhouseDemo", "Error"];
    if (isSuccsess) {
        let buf1 = Buffer.from([comandNumber.findIndex(p => p == comandString)]);
        let buf2 = Buffer.from("" + value);
        buf = Buffer.concat([buf1, buf2]);
    }
    else {
        buf = Buffer.from([comandNumber.findIndex(p => p == comandString)]);
    }
    console.log(comandString + " comand number " + comandNumber.findIndex(p => p == comandString) + " value " + value);
    return buf;
};

module.exports.increaseFrequency = (buffer) => {
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

function readAudioBuffer(buffer, byterate, index) {
	switch (byterate) {
		case 1:
			return buffer[index];
		case 2:
			return buffer.readInt16LE(index * byterate);
		case 4:
			return buffer.readInt32LE(index * byterate);
	}
}

function writeAudioBuffer(value, buffer, byterate, index) {
	switch (byterate) {
		case 1:
			return buffer[index] = value;
		case 2:
			return buffer.writeInt16LE(value, index * byterate);
		case 4:
			return buffer.writeInt32LE(value, index * byterate);
	}
}