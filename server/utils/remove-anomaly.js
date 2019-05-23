const computeWindow = (arr, n = 100, k = 2, prevSigma = -1, window = [0, 0]) => {
    if (n <= 0) {
        return window;
    }

    const arrMid = computeMid(arr);
    const sigma = computeSigma(arr, arrMid);

    const winMax = arrMid + (k * sigma);
    const winMin = arrMid - (k * sigma);
    window = [winMin, winMax];

    if (prevSigma == sigma) {
        return window;
    }

    const filtred = removeAnomaly(arr, window);

    return computeWindow(filtred, n - 1, k, sigma, window)
};

const computeSigma = (arr, arrMid) => {
    const arrMidItems = arr.map((arrItem) => {
        return Math.pow(arrItem - arrMid, 2);
    });
    return Math.sqrt(computeMid(arrMidItems));
}

const removeAnomaly = (arr, window, saveLength = true) => {
    return arr.filter((arrItem) => {
        return window[0] < arrItem && arrItem < window[1];
    });
};

const computeMid = (arr) => {
    return arr.reduce((reducer, arrItem) => {
        return arrItem + reducer;
    }, 0) / arr.length
};

module.exports = {
    computeWindow,
    removeAnomaly
}