const https = require("https");
const net = require('net');
const querystring = require("querystring");
const utils = require("./utils")
const worker = require("./worker");
const iam = "CggaATEVAgAAABKABLMZjj280o3Lc3LBrGKFF_5634yOb8GUCI7V7WYFiIhz6BKV-eVgHqmV-94JrY6rEwOFnx0hbYv5LLspi2RXjr9iR7D7XwEEAG0_XOFX42uvL4L5IVIBn-cJFUKd2nWjbNdswWeL5IUBCpsqA50l1O2KxDplyqr45ffz1EoY9QNGFMGqnO5ay0AdoN-PD6LkeJht4SWuIEnIBpxcB50WO7CU_toqw44sRBtdJUmv2O4DOBuLzonTSpYc_vM4XRsRQXtYWpyDdTNDS0HTBXFNPSgwf7uX2Q82YhQWYBJbFjl4Qy2xJ1Gv5ZK5W95oEn6h9Afrn0BinHWyNWZjEnEJ_6aK_f8GoS0AaxEncyUBIXhwOaG7jR-UAZNkIfs2WUTPwYtEzYXGnFm_lt2JjhCOBaX8JI0L2T_TGAGnjT_9XIJxP-ucbdn8lnmgnDPV18n6BXLdwKk9z6KaaSy6erz-PzBey-ALGx8FP8FSKlBLN6IP6U-SY0DK4CXFPdMuXvUGE1acs0eJ6KkeT7VLaZI-yXYjDmxYIiee0u21-Qmgk4oyTbZTgKEcmmThFyCp_v-6x05enU11hozewTLxd9GJUv9AP-Y6L4N1G1SJ48HHlD0vGAZLNkLXScm0ydKQlhi7Zv1j1hWCBqQxByd2jfKWJ7PK0AsQLlA6RwU4DuIezaC7GmoKIGEyMjJkZTg5Y2IxNzQ5NTQ5Y2IzZTBhODY2NmQxY2RjENDtoeUFGJC_pOUFIigKFGFqZWprdmtmdmtwZWl1ODJqOGc1EhBiZWxvdGVsb3Yua2lyaWxsWgAwAjgBSggaATEVAgAAAFABIPkE";//4 послдених символа засолить

// From ESP to server
const server = net.createServer((socket) => {
    let receiverBytes = 0;
    const lpcmBuffer = Buffer.alloc(48000);
    socket.on("error", function (err) {
        console.error("Error: " + err);
        socket.end(utils.genereteResponceBuffer(false, "Error"));
    });
    socket.on("data", (lpcmChunk) => {
        console.log("Received chunk");
        lpcmChunk.copy(lpcmBuffer, receiverBytes);
        receiverBytes += lpcmChunk.length;
        console.log("Total bytes received: " + receiverBytes);
        if (receiverBytes == 48000) {
            console.log("Start speech recognizing");
            const incresedLpcmBuffer = utils.increaseFrequency(lpcmBuffer);
            const queryString = querystring.stringify({
                format: "lpcm",
                sampleRateHertz: "16000",
                folderId: "b1grc8mj9t0vt1fu9n3c"
            });
            const options = {
                hostname: "stt.api.cloud.yandex.net",
                path: "/speech/v1/stt:recognize?" + queryString,
                method: "POST",
                headers: {
                    "Authorization": `Bearer ${iam}`,
                }
            };
            const req = https.request(options, (res) => {
                console.log("statusCode:", res.statusCode);
                console.log("headers:", res.headers);
                let body = "";
                res.on("data", (chunk) => {
                    console.log("Reseived speech recognizing data");
                    body += chunk;
                });
                req.on("error", (err) => {
                    socket.end(utils.genereteResponceBuffer(false, "Error"));
                    console.error("Error: " + err);
                    receiverBytes = 0;
                });
                res.on("end", function () {
                    var response = JSON.parse(body);
                    console.log("Got a response: ", response);
                    socket.end(worker(response));
                    receiverBytes = 0;
                });
            });
            req.write(incresedLpcmBuffer);
            req.end();
        }
    });
});
server.listen(8086, "46.101.116.209");
console.log("Server started");
