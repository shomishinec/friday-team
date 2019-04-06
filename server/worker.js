const utils = require("./utils")

let storehouseDemo = false;
let storhouseTaskNumber = 0;

module.exports = (data) => {
    if (data.hasOwnProperty("error_code")) {
        console.log(data);
        return utils.genereteResponceBuffer(false, "Error");
    }
    var answerWords = [];

    answerWords = data.result.split(" ");
    if (answerWords.includes("время") || answerWords.includes("таймер")) {
        console.log("I heard time or timer")
        var timeSeconds = 0;//sending in miliseconds
        if (answerWords.includes("установи") || answerWords.includes("поставь") || answerWords.includes("запусти") || answerWords.includes("ждем")) {
            console.log("I heard set")
            if (answerWords.includes("час") || answerWords.includes("часа")) {
                console.log("I heard hour")
                let index = answerWords.findIndex(p => p.includes("час")) - 1;
                timeSeconds = + answerWords[index] * 60 * 60 * 1000;
            }
            if (answerWords.includes("минуту") || answerWords.includes("минут") || answerWords.includes("минуты")) {
                console.log("I heard minutes")
                let index = answerWords.findIndex(p => p.includes("минут")) - 1;
                timeSeconds = + answerWords[index] * 60 * 1000;
            }

            if (answerWords.includes("секунд") || answerWords.includes("секунды")) {
                console.log("I heard seconds")
                let index = answerWords.findIndex(p => p.includes("секунд")) - 1;
                timeSeconds = + answerWords[index] * 1000;
            }
        }
        storhouseTaskNumber = 0;
        storehouseDemo = false;
        return genereteResponceBuffer(true, "startTimer", timeSeconds);
    }
    if (storehouseDemo || answerWords.includes("склад") || answerWords.includes("склада") || answerWords.includes("хранилище") || answerWords.includes("складом")) {
        if (!storehouseDemo) {
            console.log("I heard storhouse demo")
            return genereteResponceBuffer(true, "storhouseDemo", "task or question");
        }
        storehouseDemo = true;
        if (answerWords.includes("дальше") || answerWords.includes("следующая") || answerWords.includes("есть") || answerWords.includes("где") || answerWords.includes("задача") || answerWords.includes("задание")) {
            console.log("I heard next")
            return genereteResponceBuffer(true, "storhouseDemo", storehouseTasks())
        }
    }


};
var storehouseTasks = () => {
    task = ["row 6 shelf 8", "row 4 shelf 3", "row 3 shelf 2", "row 9 shelf 4", "row 7 shelf 5", "you are smart"]
    storhouseTaskNumber++;
    if (task.length == storhouseTaskNumber) {
        storhouseTaskNumber = 0;
        storehouseDemo = false;
    }
    return task[storhouseTaskNumber - 1];
};