'use strict';

class SignatureStore {
  constructor() {
    this.kStorageKey = 'SignatureStore';
    this.signatures = [];
    this.restore();
  }
  restore() {
    let str = window.localStorage.getItem(this.kStorageKey);
    if (str == null)
      return;
    let obj = JSON.parse(str);
    if (obj.version != 1) {
      console.log(`Unsure how to load signatures with version ${obj.version}`);
      return;
    }
    this.signatures = obj.signatures;
  }
  save() {
    let obj = {'version': 1, 'signatures': this.signatures};
    window.localStorage.setItem(this.kStorageKey, JSON.stringify(obj));
  }
  length() {
    return this.signatures.length;
  }
  addSignature(path, width, height) {
    this.signatures.push([path, width, height]);
    this.save();
  }
  deleteSignature(path, width, height) {
    let index = this.signatures.findIndex((elt) => {
      return elt[1] == width && elt[2] == height && elt[0] == path;
    });
    if (index < 0)
      return;
    this.signatures.splice(index, 1);
    this.save();
  }
  getSignature(index) {
    return this.signatures[index];
  }
  getAsSVG(index) {
    let ret = this.getSignature(index);
    if (!ret)
      return ret;
    return ['<svg viewBox="0 0 ', ret[1], ' ', ret[2],
            '" width="200" xmlns="http://www.w3.org/2000/svg"><path d="',
            ret[0],
            '" stroke="none" fill="black" fill-rule="evenodd" /></svg>'
           ].join('');
  }
}
