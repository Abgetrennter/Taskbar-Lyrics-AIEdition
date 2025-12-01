"use strict";

/**
 * @module 工具函数
 * @description 提供通用的工具函数，如对象扁平化、防抖、延时等
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

const Utils = {
    /**
     * 扁平化对象
     * @description 将嵌套的对象转换为单层对象，键名使用前缀拼接
     * @param {Object} obj - 需要扁平化的对象
     * @param {string} [prefix=''] - 键名前缀
     * @returns {Object} 扁平化后的对象
     */
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
    
    /**
     * 防抖函数
     * @description 限制函数在一定时间内只能执行一次
     * @param {Function} func - 需要执行的函数
     * @param {number} wait - 等待时间（毫秒）
     * @returns {Function} 包装后的函数
     */
    debounce: (func, wait) => {
        let timeout;
        return function(...args) {
            const context = this;
            clearTimeout(timeout);
            timeout = setTimeout(() => func.apply(context, args), wait);
        };
    },

    /**
     * 延时函数
     * @description 返回一个Promise，在指定时间后resolve
     * @param {number} ms - 延时时间（毫秒）
     * @returns {Promise} Promise对象
     */
    delay: (ms) => {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
};
