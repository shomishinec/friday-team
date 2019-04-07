const https = require("https");
const net = require('net');
const querystring = require("querystring");
const utils = require("./utils")
const worker = require("./worker");
const fileSaver = require("./fileSaver");
const iam = "CggaATEVAgAAABKABI1ng-nTwBRod6IVxRlQyhTUbTE79EscsLBvYVJekryfuzVTPALBqRSuLqpPt6mFUcCKt8DuPPm8UOeWLT1u5lIm5UvAf-IfsmKIvafrXx5kU17O3x4eTYYPmau7BeFgiHnPvILwlzrpkCb9uYIgHgnx9rv3zIqCoGo754_O-3NjhZ7E7_s_DK4nlnb2reyh_iuOYZatutVm18xRd7fKNrbheYCA3GgryVmroot3sX0bqLAU2XoMKlAEMna5eOcH8LBmczwWwgmEB-GNxP_WuX7a5Z-FO0GYt25HvbKBtLcu6N0FNOIp205Hj8WGoULa6EoXMjlQClSXwEu5ol_pPz2JRpf8a-AgKAOZVrSUf41C3dbVa2cUZZDK1LW-8I4kd0j_BFLl5zYSBqupR_V-wH6TORY-JOr6oh8CYrgsOWtDf16ylQkFb08906HfuEIUUW9a4D1INuOkbNuV6I79jHXTqm1QTabx2iFe7uPJjz9y0LZPYKao6sy0tigRztQmWZfhURzXjUTuHA7bSkUK3rFxKSHgY6thUvZQqhbKrtDY-RxErQf64n0pZ6y5f0eA-hunxKoxiyEBP4mrp0j7Us1KYBanNQfcpVosm77ej0kS2yLcHcG-wImGqrlWhH9JTYh-EDAYqOOL_0y_661h2wDeQej8hCmCbyfuZ516KyGTGmoKIDQxZGQ1NDA3ZTViNzRjNjZiMDVjMTJlYzFiMDU1M2NhEIO9puUFGMOOqeUFIigKFGFqZWprdmtmdmtwZWl1ODJqOGc1EhBiZWxvdGVsb3Yua2lyaWxsWgAwAjgBSggaATEVAgAAAFABIPkE";//4 послдених символа засолить

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
            // fileSaver.save(lpcmBuffer);
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
                    // TODO command not recognize
                    socket.end(worker(response));
                    receiverBytes = 0;
                });
            });
            req.write(incresedLpcmBuffer);
            req.end();
        }
    });
});
server.listen(8086, "172.20.10.3");
console.log("Server started");
