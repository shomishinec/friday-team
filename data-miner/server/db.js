const fs = require("fs");
const config = require("./config");
const classes = require("./../shared/classes");

let inMemoryDB = null;
let onDBChanged = null;

const getDB = () => {
    if (fs.existsSync(config.dbFileName)) {
        const json = fs.readFileSync(config.dbFileName, "utf8");
        inMemoryDB = classes.DB.fromDto(JSON.parse(json));
    } else {
        inMemoryDB = new classes.DB([], getData());
    }
    return inMemoryDB;
};

const setDB = (db) => {
    fs.writeFileSync(config.dbFileName, JSON.stringify(db), "utf8");
};

const dbChanged = () => {
    onDBChanged && onDBChanged();
};

const updateDB = (newIds) => {
    const db = getDB();
    const newDataItems = newIds.map((id) => {
        const frames = fs.readdirSync(`${config.watchDirectory}/${id}`)
            .map(frameFileName => parseInt(frameFileName.split('.')[1]))
            .map(frameId => new classes.DataItemFrame(frameId, []));
        return new classes.DataItem(id, 3000, 96044, false, frames);
    });
    db.dataItems = db.dataItems.concat(newDataItems);
    setDB(db);
    dbChanged();
};

const watch = () => {
    const dirsAndFiles = fs.readdirSync(config.watchDirectory);
    const ids = dirsAndFiles
        .map(name => parseInt(name.split(".")[0]))
        .filter((value, index, self) => self.indexOf(value) === index);
    const dataItems = getDB().dataItems;
    const existIds = dataItems.map(dataItem => dataItem.id);
    const newIds = ids.filter(id => existIds.indexOf(id) === -1);
    if (newIds.length !== 0) {
        updateDB(newIds);
    }
};

setInterval(watch, config.watchFSTimeout);

module.exports = {
    getDB,
    setDB,
    dbChanged
}
