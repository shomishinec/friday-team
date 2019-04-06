const fs = require("fs");
const https = require("https");
var net = require('net');
const querystring = require("querystring");

const iam = "CggaATEVAgAAABKABLMZjj280o3Lc3LBrGKFF_5634yOb8GUCI7V7WYFiIhz6BKV-eVgHqmV-94JrY6rEwOFnx0hbYv5LLspi2RXjr9iR7D7XwEEAG0_XOFX42uvL4L5IVIBn-cJFUKd2nWjbNdswWeL5IUBCpsqA50l1O2KxDplyqr45ffz1EoY9QNGFMGqnO5ay0AdoN-PD6LkeJht4SWuIEnIBpxcB50WO7CU_toqw44sRBtdJUmv2O4DOBuLzonTSpYc_vM4XRsRQXtYWpyDdTNDS0HTBXFNPSgwf7uX2Q82YhQWYBJbFjl4Qy2xJ1Gv5ZK5W95oEn6h9Afrn0BinHWyNWZjEnEJ_6aK_f8GoS0AaxEncyUBIXhwOaG7jR-UAZNkIfs2WUTPwYtEzYXGnFm_lt2JjhCOBaX8JI0L2T_TGAGnjT_9XIJxP-ucbdn8lnmgnDPV18n6BXLdwKk9z6KaaSy6erz-PzBey-ALGx8FP8FSKlBLN6IP6U-SY0DK4CXFPdMuXvUGE1acs0eJ6KkeT7VLaZI-yXYjDmxYIiee0u21-Qmgk4oyTbZTgKEcmmThFyCp_v-6x05enU11hozewTLxd9GJUv9AP-Y6L4N1G1SJ48HHlD0vGAZLNkLXScm0ydKQlhi7Zv1j1hWCBqQxByd2jfKWJ7PK0AsQLlA6RwU4DuIezaC7GmoKIGEyMjJkZTg5Y2IxNzQ5NTQ5Y2IzZTBhODY2NmQxY2RjENDtoeUFGJC_pOUFIigKFGFqZWprdmtmdmtwZWl1ODJqOGc1EhBiZWxvdGVsb3Yua2lyaWxsWgAwAjgBSggaATEVAgAAAFABIPkE";//4 послдених символа засолить

let fileBuffer;
const fileChunks = [];
var storehouseDemo = false;
var storhouseTaskNumber=0;
var server = net.createServer((soket)=>{
        soket.on("data", (fileChunk)=>{
            fileChunks.push(fileChunk);
            fileBuffer = Buffer.concat(fileChunks);
            const contentLength = fileBuffer.length - 40;
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
        
                var body = "";
        
                res.on("data", (chunk) => {
                    body += chunk;
                });
        
                req.on("error", (e) => {
                    soket.write(genereteResponceBuffer(false, "Error"));
                    console.error(e);
                });
        
                res.on("end", function () {
                    var response = JSON.parse(body);
                    soket.write(worker(response));
                    console.log("Got a response: ", response);
                });
            });
        
            req.write(fileBuffer.slice(40, contentLength));
            req.end();
            
        } )

    
});
server.listen(8086, "46.101.116.209");
var worker = (data)=>{
    if (data.hasOwnProperty("error_code")){
        console.log(data);
       return genereteResponceBuffer(false, "Error");
    }
    var answerWords = [];
    
    answerWords = data.result.split(" ");
    if(answerWords.includes("время")||answerWords.includes("таймер"))
    {
        console.log("I heard time or timer")
        var  timeSeconds=0;//sending in miliseconds
        if(answerWords.includes("установи")||answerWords.includes("поставь")||answerWords.includes("запусти")||answerWords.includes("ждем"))
        {   
            console.log("I heard set")
             if(answerWords.includes("час")||answerWords.includes("часа"))
            {
                console.log("I heard hour")
                let index = answerWords.findIndex(p=> p.includes("час"))-1;
                timeSeconds =+ answerWords[index]*60*60*1000;
            }
            if(answerWords.includes("минуту")||answerWords.includes("минут")||answerWords.includes("минуты"))
            {
                console.log("I heard minutes")
                let index = answerWords.findIndex(p=> p.includes("минут"))-1;
                timeSeconds =+ answerWords[index]*60*1000;
            }
        
            if(answerWords.includes("секунд")||answerWords.includes("секунды"))
            {
                console.log("I heard seconds")
                let index = answerWords.findIndex(p=> p.includes("секунд"))-1;
                timeSeconds =+ answerWords[index]*1000;
            }
        }
        storhouseTaskNumber = 0;
        storehouseDemo = false;
        return genereteResponceBuffer(true, "startTimer", timeSeconds);
    }
    if(storehouseDemo||answerWords.includes("склад")||answerWords.includes("склада")||answerWords.includes("хранилище")||answerWords.includes("складом")){
        if(!storehouseDemo)
        {
            console.log("I heard storhouse demo")
            return genereteResponceBuffer(true, "storhouseDemo", "task or question");
        }
        storehouseDemo=true;
        if(answerWords.includes("дальше")||answerWords.includes("следующая")||answerWords.includes("есть")||answerWords.includes("где")||answerWords.includes("задача")||answerWords.includes("задание"))
            {
                console.log("I heard next")
                return genereteResponceBuffer(true, "storhouseDemo", storehouseTasks())
            }
    }


};
var storehouseTasks = ()=>{
    task = ["row 6 shelf 8","row 4 shelf 3","row 3 shelf 2","row 9 shelf 4","row 7 shelf 5","you are smart"]
    storhouseTaskNumber++;
    if (task.length==storhouseTaskNumber)
    {
        storhouseTaskNumber = 0;
        storehouseDemo = false;
    }
    return task[storhouseTaskNumber-1];
};
var genereteResponceBuffer = (isSuccsess, comandString, value=null)=>{
    var buf = Buffer.alloc(256);

    var comandNumber= ["godMode", "startTimer", "storhouseDemo", "Error" ];
    if(isSuccsess)
    {
        let buf1= Buffer.from([comandNumber.findIndex(p=>p==comandString)]);
        let buf2= Buffer.from(""+value);
        buf= Buffer.concat([buf1, buf2]);
    }
    else{
        buf = Buffer.from([comandNumber.findIndex(p=>p==comandString)]);
    }
    console.log(comandString+" comand number "+comandNumber.findIndex(p=>p==comandString)+ " value "+value);
    return buf;

};
