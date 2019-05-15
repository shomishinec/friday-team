const https = require("https");
const net = require('net');
const querystring = require("querystring");
const config = require("config");
const utils = require("./utils")
const worker = require("./worker");
const fileSaver = require("./fileSaver");
const iam = "CggaATEVAgAAABKABB9mBv8qJkhBC6PXXJSEuJDud3xiHbCZNEWrezJbxsuCWuvEDZAMORe2VOEHihB2v_6YZLVn_er_YaldsniDpC95DB2IhIVYrC5goNnUBRXLzFHSRKIlmuIEsQoRq5EOcdP9H0LsoI60TOmzPAseX58MDsQIlFaVbSAZs0P9F4HW_HlnhG79qV5LPM8Kfxkkg-Vfmbv3YSaDEg4IXMHvZ6Z3TEkXScZLpSEwoURAboJJH4oniERemQ7ZhDwT8fv47fpSGba4H1WaclFltSWDJeVonOFrJvzVGsDnheIIn-miN-AdCk-NA3VlA87ZEIvmU8eyCt-x-6kNAWMBpYkEa8oV3ZBbYwaa7Wol-bEeBvnRKAYAO3MtqVyczrQKC0L3NyOqs6bE6V24Xq9l03YTwqCsUqHcIKJJrG8Ag-GDQiPhn8z4iqy69DVrNnjE877AXMNwKWTTGhQGjBKmwCeYB1qVnPgwW5QgvAFaxQBE0UrjuwD2XO-bacnsoiiRHmGl6X9ptJHH8zy__2Nxs5vq2QtX3UzPrRPSXaMsMSK2_0HFoEqQADhosBJafrjomlmV9f_ErWxn3RXsQ1J7BY6m0ZTmbDffifGksK4-RVEo6myoIUOIBlns0YI2BnmGqMTHgwvM4o_6EJr2ZSScbuRc3rh8HYxQrxF_MHvHSN7O9iRmGmoKIDg3YjdkZTM3NjljNDQ2YzdiM2FmODNiMTZmYjM5MDJiEPDpq-UFGLC7ruUFIigKFGFqZWprdmtmdmtwZWl1ODJqOGc1EhBiZWxvdGVsb3Yua2lyaWxsWgAwAjgBSggaATEVAgAAAFABIPkE";

// From ESP to server
const server = net.createServer((socket) => {
    let receiverBytes = 0;
    const lpcmBuffer = Buffer.alloc(48000);
    const timer = setTimeout(10000, () => {
        socket.end(utils.genereteResponceBuffer(false, "Error"));
    })
    socket.on("error", function (err) {
        console.error("Error: " + err);
        clearTimeout(timer);
        socket.end(utils.genereteResponceBuffer(false, "Error"));
    });
    socket.on("data", (lpcmChunk) => {
        console.log("Received chunk");
        lpcmChunk.copy(lpcmBuffer, receiverBytes);
        receiverBytes += lpcmChunk.length;
        console.log("Total bytes received: " + receiverBytes);
        if (receiverBytes == 48000) {
            clearTimeout(timer);
            console.log("Start speech recognizing");
            fileSaver.save(lpcmBuffer);
            // const incresedLpcmBuffer = utils.increaseFrequency(lpcmBuffer);
            // const queryString = querystring.stringify({
            //     format: "lpcm",
            //     sampleRateHertz: "16000",
            //     folderId: "b1grc8mj9t0vt1fu9n3c"
            // });
            // const options = {
            //     hostname: "stt.api.cloud.yandex.net",
            //     path: "/speech/v1/stt:recognize?" + queryString,
            //     method: "POST",
            //     headers: {
            //         "Authorization": `Bearer ${iam}`,
            //     }
            // };
            // const req = https.request(options, (res) => {
            //     console.log("statusCode:", res.statusCode);
            //     console.log("headers:", res.headers);
            //     let body = "";
            //     res.on("data", (chunk) => {
            //         console.log("Reseived speech recognizing data");
            //         body += chunk;
            //     });
            //     req.on("error", (err) => {
            //         socket.end(utils.genereteResponceBuffer(false, "Error"));
            //         console.error("Error: " + err);
            //         receiverBytes = 0;
            //     });
            //     res.on("end", function () {
            //         var response = JSON.parse(body);
            //         console.log("Got a response: ", response);
            //         // TODO command not recognize
            //         socket.end(worker(response));
            //         receiverBytes = 0;
            //     });
            // });
            // req.write(incresedLpcmBuffer);
            // req.end();
        }
    });
});
server.listen(config.port, config.address);
console.log("Server started");
