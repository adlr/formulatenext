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
  addSignature(path) {
    this.signatures.push(path);
    this.save();
  }
  deleteSignature(index) {
    this.signatures.splice(index, 1);
    this.save();
  }
  getSignature(index) {
    return this.signatures[index];
  }
}
