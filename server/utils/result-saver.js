const fs = require("fs");
const fft = require("fft-js").fft;
const fftUtil = require("fft-js").util;
const bmp = require("bmp-js");
const commonUtils = require("./common");
const removeAnomaly = require("./remove-anomaly")

const makeDir = (folderName) => {
	if (!fs.existsSync(folderName)) {
		fs.mkdirSync(folderName);
	}
};

const writeWavFile = (fileName, buffer, frequency, bitrate) => {
	fileName = fileName + "." + frequency + "kHz." + bitrate + "bit.wav"
	outStream = fs.createWriteStream(fileName);

	const headerBuffer = new Buffer.alloc(1024);
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

	console.log("Audio " + fileName + " writen");
};

const saveAudio = (buffer, fileName, frequency = 16000, byterate = 2) => {
	makeDir("output");
	writeWavFile("output/" + fileName, buffer, frequency / 1000, byterate * 8);
};

const saveAudioFrames = (buffer, originalFileName, frameDuration = 50, frequency = 16000, byterate = 2) => {
	const bytesPerSecond = frequency * byterate;
	const bytesPerMillis = bytesPerSecond / 1000;
	const sampleDuration = buffer.length / bytesPerSecond;
	const framePerSecond = 1000 / frameDuration
	const framesCount = (framePerSecond * sampleDuration * 2) - 1;
	const frameOffset = frameDuration / 2 * bytesPerMillis;
	makeDir("output");
	makeDir("output/" + originalFileName);
	for (let i = 0; i < framesCount; i++) {
		const frameBuffer = Buffer.alloc(bytesPerMillis * frameDuration);
		buffer.copy(frameBuffer, 0, i * frameOffset, i * frameOffset + bytesPerMillis * frameDuration - 1);
		const extendedFrameBuffer = Buffer.alloc(bytesPerMillis * sampleDuration * 1000);
		for (let j = 0; j < sampleDuration * 1000 / frameDuration; j++) {
			frameBuffer.copy(extendedFrameBuffer, j * frameDuration * bytesPerMillis, 0, bytesPerMillis * frameDuration);
		}
		writeWavFile(`output/${originalFileName}/${originalFileName}.${commonUtils.pad(i + 1, 3)}`, extendedFrameBuffer, frequency / 1000, byterate * 8);
	}
};

const saveExcelPCMData = (buffer, fileName, byterate = 2) => {
	fileName = "output/" + fileName + ".pcm-data.csv";
	let values = "";
	for (let i = 0; i < buffer.length / 2; i++) {
		values += i + "," + commonUtils.readAudioBuffer(buffer, byterate, i).toString() + "\n";
	}
	makeDir("output");
	fs.writeFileSync(fileName, values);
	console.log("PCM data " + fileName + " writen");
};

const saveSpectrogram = (buffer, fileName, itemsInFrame, frequency = 16000, byterate = 2) => {
	fileName = "output/" + fileName + ".spectrogramm.bmp";
	const framesCount = Math.floor(buffer.length / byterate / itemsInFrame);
	const maxNormalAmplitude = 4096;
	const columns = [];
	for (let i = 0; i < framesCount; i++) {
		const startIndex = i * itemsInFrame;
		const sample = [];
		for (let j = 0; j < itemsInFrame; j++) {
			sample.push(commonUtils.readAudioBuffer(buffer, byterate, startIndex + j));
		}
		const phasors = fft(sample);
		const frequencies = fftUtil.fftFreq(phasors, 16000); // Sample rate and coef is just used for length, and frequency step
		const magnitudes = fftUtil.fftMag(phasors);
		let both = frequencies.map(function (f, ix) {
			return { frequency: f, magnitude: magnitudes[ix] };
		});
		columns.push(both);
	}

	const allMagnitudes = [];
	columns.forEach((column, index) => {
		column.forEach((point) => {
			if (index == 0) {
				return;
			}
			allMagnitudes.push(point.magnitude);
		});
	});
	const magnitudesWindow = removeAnomaly.computeWindow(allMagnitudes, 10, 5);
	let maxMagnitude = 0;
	columns.forEach((column) => {
		column.forEach((point) => {
			if (maxMagnitude < point.magnitude) {
				if (point.magnitude >= magnitudesWindow[1]) {
					maxMagnitude = magnitudesWindow[1];
				} else {
					maxMagnitude = point.magnitude;
				}
			}
		});
	});
	const magnitudeK = maxNormalAmplitude / maxMagnitude;
	const width = columns.length;
	const height = columns[0].length;
	const colorPallete = bmp.decode(fs.readFileSync("spectrogramm/color-palette.bmp"));
	const bitMapBuffer = Buffer.alloc(width * height * 4);
	let bufferLength = 0;
	for (let i = height - 1; i >= 0; i--) {
		for (let j = 0; j < width; j++) {
			let magnitude = columns[j][i].magnitude;
			if (magnitude > magnitudesWindow[1]) {
				magnitude = magnitudesWindow[1]
			}
			if (magnitude < magnitudesWindow[0]) {
				magnitude = magnitudesWindow[0]
			}
			magnitude = Math.floor(magnitude * magnitudeK) - 1;
			bitMapBuffer[bufferLength] = 255;
			bitMapBuffer[bufferLength + 1] = colorPallete.data[magnitude * 4 + 1]
			bitMapBuffer[bufferLength + 2] = colorPallete.data[magnitude * 4 + 2]
			bitMapBuffer[bufferLength + 3] = colorPallete.data[magnitude * 4 + 3]
			bufferLength += 4;
		}
	}
	const bitMapFile = {
		bitPP: 32,
		data: bitMapBuffer,
		width,
		height
	};

	const rawData = bmp.encode(bitMapFile);
	makeDir("output");
	fs.writeFileSync(fileName, rawData.data);
	console.log("Spectrogram " + fileName + " writen");
};

module.exports = {
	saveAudio,
	saveAudioFrames,
	saveExcelPCMData,
	saveSpectrogram
};