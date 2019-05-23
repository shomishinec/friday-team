const fs = require("fs");
const net = require("net");

function increaseFrequency(buffer, frequency, newFrequency, byterate) {
	const k = newFrequency / frequency;
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

function interpolate(buffer, byterate, pow) {
	const offset = (pow - 1) / 2;
	for (let i = offset + 1; i < buffer.length / byterate - offset; i += byterate) {
		let newValue = 0;
		let divider = 0;
		for (let j = -offset; j <= offset; j += byterate) {
			newValue += readAudioBuffer(buffer, byterate, i + j) * (offset - Math.abs(j));
			divider += (offset - Math.abs(j));
		}
		writeAudioBuffer(newValue / divider, buffer, byterate, i);
	}
	return buffer;
}

function writeFile(fileName, buffer, frequency, bitrate) {
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
	headerBuffer.writeUInt16LE(CHANELS_COUNT * bitrate / 8, 32);

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

function saveExperemantalResults(buffer) {
	const fileName = (+ new Date());
	let frequency = INIT_FREQUENCY;
	let byterate = INIT_BYTERATE;
	let bitrate = INIT_BITRATE;

	// writeFile(fileName, buffer, frequency, bitrate);
	// logFile(buffer, byterate);

	buffer = increaseFrequency(buffer, frequency, FREQUENCY, byterate);
	frequency = FREQUENCY;
	writeFile(fileName, buffer, frequency, bitrate);
	// logFile(buffer, byterate);

	// buffer = interpolate(buffer, byterate, 17);
	// writeFile(fileName + ".interpolated", buffer, frequency, bitrate);
	// logFile(buffer, byterate);
}

function logFile(buffer, byterate) {
	let result = "";
	for (i = 0; i < buffer.length / byterate; i += byterate) {
		result += readAudioBuffer(buffer, byterate, i) + ", ";
	}
	console.log(result);
}

function onServerCreated(socket) {
	console.log("Client connected");
	const buffer = new Buffer.alloc(INIT_FREQUENCY * 1000 * INIT_BYTERATE * DURATION);
	let bufferLength = 0;
	const timeout = setTimeout(() => {
		socket.end();
		console.log("Client disconnected by timeout");
		saveExperemantalResults(buffer);
	}, 30000);
	socket.on("end", function () {
		clearTimeout(timeout);
		console.log("Client disconnected");
		saveExperemantalResults(buffer);
	});
	socket.on("error", function (err) {
		console.error("Error! " + err);
	});
	socket.on("data", function (receivedBuffer) {
		console.log("Data received");
		try {
			receivedBuffer.copy(buffer, bufferLength);
			bufferLength += receivedBuffer.length;
			console.log("Got chunk of " + receivedBuffer.length + " bytes, total is " + bufferLength + " bytes");
		}
		catch (ex) {
			console.error("Error! " + ex);
		}
	});
}

const server = net.createServer(onServerCreated);

server.listen({
	host: "172.20.10.3",
	port: 3000,
	exclusive: true
}, () => {
	console.log("Opened server on", server.address());
});