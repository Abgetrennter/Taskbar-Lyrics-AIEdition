"use strict";

const Utils = {
    flattenObject: (obj, prefix = '') => {
        return Object.keys(obj).reduce((acc, k) => {
            const pre = prefix.length ? prefix + '_' : '';
            if (typeof obj[k] === 'object' && obj[k] !== null && !Array.isArray(obj[k]))
                Object.assign(acc, Utils.flattenObject(obj[k], pre + k));
            else
                acc[pre + k] = obj[k];
            return acc;
        }, {});
    },
    
    debounce: (func, wait) => {
        let timeout;
        return function(...args) {
            const context = this;
            clearTimeout(timeout);
            timeout = setTimeout(() => func.apply(context, args), wait);
        };
    },

    delay: (ms) => {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
};
