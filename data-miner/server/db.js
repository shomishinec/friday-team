const fs = require("fs");
const config = require("./config");
const classes = require("./../shared/classes");

let inMemoryDB = null;

module.exports.getDB = () => {
    if (fs.existsSync(config.dbFileName)) {
        const json = fs.readFileSync(config.dbFileName, "utf8");
        inMemoryDB = classes.DB.fromDto(JSON.parse(json));
    } else {
        inMemoryDB = new classes.DB([], getData());
    }
    return inMemoryDB;
};

module.exports.onDataChanged = () => {

};

module.exports.watchData = () => {

};