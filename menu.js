'use strict';

let menuOpen = function(ev) {
  let ev2 = document.createEvent('MouseEvents');
  ev2.initEvent('click', true, true);
  document.getElementById('file-input').dispatchEvent(ev2);
};
let menuClose = function() { console.log('menuClose'); };
let menuSave = function() {
  if (DownloadFile)
    DownloadFile();
};
let menuSaveAs = function() { console.log('menuSaveAs'); };
let menuPrint = function() { console.log('menuPrint'); };
let menuUndo = function() { console.log('menuUndo'); };
let menuRedo = function() { console.log('menuRedo'); };
let menuCut = function() { console.log('menuCut'); };
let menuCopy = function() { console.log('menuCopy'); };
let menuPaste = function() { console.log('menuPaste'); };
let menuZoomOut = function() { zoomOut(); };
let menuZoomIn = function() { zoomIn(); };
let menuView100 = function() { console.log('menuView100'); };

let menus = [
  {name: "File", children: [
    {name: "Open...", key: "o", action: menuOpen},
    {name: "-"},
    {name: "Close", key: "w", action: menuClose},
    {name: "Save", key: "s", action: menuSave},
    {name: "Save As...", key: "S", action: menuSaveAs},
    {name: "-"},
    {name: "Print...", key: "P", action: menuPrint}
  ]},
  {name: "Edit", children: [
    {name: "Undo", key: "z", action: menuUndo},
    {name: "Redo", key: "Z", action: menuRedo, enabled: false},
    {name: "-"},
    {name: "Cut", key: "x", action: menuCut},
    {name: "Copy", key: "c", action: menuCopy},
    {name: "Paste", key: "v", action: menuPaste},
  ]},
  {name: "View", children: [
    {name: "Zoom", children: [
      {name: "Out", key: "-", action: menuZoomOut},
      {name: "In", key: "=", action: menuZoomIn},
      {name: "100%", key: "0", action: menuView100}
    ]},
    {name: "Zoom2", children: [
      {name: "Out", key: "-", action: menuZoomOut},
      {name: "In", children: [
        {name: "Out", key: "-", action: menuZoomOut},
        {name: "In", key: "=", action: menuZoomIn},
        {name: "100%", key: "0", action: menuView100}
      ]},
      {name: "100%", key: "0", action: menuView100}
    ]}
  ]}
];

class ShortCutListener {
  constructor() {
    this.shortcuts = {};
    document.addEventListener('keydown', this.handleKeyPress.bind(this));
  }
  handleKeyPress(ev) {
    if (ev.metaKey || ev.altKey || !ev.ctrlKey || ev.key.length !== 1)
      return;
    console.log(ev.type + " " + ev.bubbles + " " + ev.key);
    const shortcut = ev.shiftKey ? ev.key.toUpperCase() : ev.key;
    if (this.shortcuts.hasOwnProperty(shortcut)) {
      ev.preventDefault();
      this.shortcuts[shortcut](ev);
    }
  }
  add(key, callback) {
    this.shortcuts[key] = callback;
  }
  remove(key) {
    console.log('removing ' + key);
    delete this.shortcuts[key];
  }
}
let shortCutListener = null;

class MenuItem {
  constructor(str, shortcut, callback) {
    this.str = str;
    this.shortcut = shortcut;
    this.callback = callback;
    this.enabled = true;
    this.child = null;
    this.div = document.createElement('div');
    if (!this.isSpacer()) {
      this.div.classList.add('menuitem');
      this.leftDiv = document.createElement('div');
      this.leftDiv.classList.add('menuitemname');
      this.div.appendChild(this.leftDiv);
      this.rightDiv = document.createElement('div');
      this.rightDiv.classList.add('menuitemright');
      this.div.appendChild(this.rightDiv);
    }
    this.updateDom();
    shortCutListener.add(shortcut, callback)
  }
  name() {
    return this.str;
  }
  setEnabled(en) {
    this.enabled = en;
    if (!this.enabled) {
      shortCutListener.remove(this.shortcut);
      this.div.classList.add('menuiteminactive');
    } else {
      shortCutListener.add(this.shortcut, this.callback);
      this.div.classList.remove('menuiteminactive');
    }
  }
  setCallback(callback) {
    if (this.callback === callback)
      return;
    this.callback = callback;
    if (this.enabled) {
      shortCutListener.remove(this.shortcut);
      shortCutListener.add(this.shortcut, this.callback);
    }
  }
  isSpacer() {
    return this.str === '-';
  }
  updateDom() {
    if (this.isSpacer()) {
      this.div.classList.add('menuspacer');
      return;
    }
    this.leftDiv.innerHTML = this.str;
    if (this.shortcut)
      this.rightDiv.innerHTML = shortcutKeyToStr(this.shortcut);
    else if (this.child)
      this.rightDiv.innerHTML = '&#9658;';
    else
      this.rightDiv.innerHTML = '';
  }
  setContainer(container) {
    if (this.child)
      this.child.div.remove();
    this.child = container;
    this.div.appendChild(this.child.div);
  }
  findMenu(path) {
    if (path.length === 0)
      return this;
    if (!this.child)
      return null;
    path.shift();
    return this.child.findMenu(path);
  }
}

class MenuContainer {
  constructor() {
    this.items = [];
    this.div = document.createElement('div');
    this.div.classList.add('submenucontainer');
    this.div.addEventListener('click', ev => { ev.stopPropagation(); });
  }
  addMenuItem(item) {
    this.items.push(item);
    this.div.appendChild(item.div);
  }
  findMenu(path) {
    for (let i = 0; i < this.items.length; i++) {
      if (this.items[i].name() === path[0]) {
        path.shift();
        return this.items[i].findMenu(path);
      }
    }
    return null;
  }
}

class MenuBarLabel {
  constructor(str) {
    this.str = str;
    this.div = document.createElement('div');
    this.div.classList.add('rootmenuitem');
    this.div.innerHTML = str;
    this.container = null;
    this.handleClick = null;
    this.domClickHandler = ev => {
      console.log('label got a click');
      this.handleClick(this);
      ev.stopPropagation();
    };
    this.handleHover = null;
    this.domEnterHoverHandler = ev => { this.onHover(ev, true); };
    this.domLeaveHoverHandler = ev => { this.onHover(ev, false); };
  }
  name() {
    return this.str;
  }
  setOnClick(handleClick) {
    if (this.handleClick != handleClick) {
      if (handleClick)
        this.div.addEventListener('click', this.domClickHandler);
      else
        this.div.removeEventListener('click', this.domClickHandler);
    }
    this.handleClick = handleClick;
  }
  onHover(ev, on) {
    if (this.handleHover)
      this.handleHover(this, on);
  }
  setOnHover(handleHover) {
    if (this.handleHover != handleHover) {
      if (handleHover) {
        this.div.addEventListener('mouseenter', this.domEnterHoverHandler);
        this.div.addEventListener('mouseenter', this.domLeaveHoverHandler);
      } else {
        this.div.removeEventListener('mouseenter', this.domEnterHoverHandler);
        this.div.removeEventListener('mouseenter', this.domLeaveHoverHandler);
      }
    }
    this.handleHover = handleHover;
  }
  setContainer(container) {
    if (this.container) {
      this.container.div.remove();
    }
    this.container = container;
    this.div.appendChild(this.container.div);
  }
  setChildVisible(vis) {
    if (vis) {
      this.div.classList.add('rootmenuitemactive');
    } else {
      this.div.classList.remove('rootmenuitemactive');
    }
    this.container.div.style.display = vis ? 'block' : 'none';
  }
  findMenu(path) {
    if (path.length === 0)
      return this;
    if (!this.container)
      return null;
    return this.container.findMenu(path);
  }
}

class MenuBar {
  constructor(div) {
    this.div = div;
    this.div.classList.add('menubar');
    this.labels = [];
    this.menuShowing = null;
    this.docClick = ev => {
      //console.log('doc click');
      this.hideMenu(); };
    document.addEventListener('click', this.docClick);
    this.div.addEventListener('click', ev => { ev.stopPropagation(); });
  }
  addLabel(label) {
    this.labels.push(label);
    this.div.appendChild(label.div);
    label.setOnClick(label => { this.labelClicked(label); });
    label.setOnHover(this.labelHover.bind(this));
  }
  showMenu(label) {
    this.menuShowing = label;
    for (let i = 0; i < this.labels.length; i++) {
      this.labels[i].setChildVisible(this.labels[i] === label);
    }
  }
  hideMenu() {
    if (!this.menuShowing)
      return;
    this.menuShowing.setChildVisible(false);
    this.menuShowing = null;
  }
  labelClicked(label) {
    if (this.menuShowing == label) {
      this.menuShowing = null;
      label.setChildVisible(false);
    } else {
      this.showMenu(label);
    }
  }
  labelHover(label, on) {
    if (!this.menuShowing || !on)
      return;
    // hovering on a menu, with a menu showing. Show what we're hoving on.
    this.showMenu(label);
  }
  findMenu(path) {
    for (let i = 0; i < this.labels.length; i++) {
      if (this.labels[i].name() === path[0]) {
        path.shift();
        return this.labels[i].findMenu(path);
      }
    }
    return null;
  }
}

const populate2 = function(items, container) {
  for (let i = 0; i < items.length; i++) {
    const initem = items[i];
    //console.log('handling ' + initem.name);
    let item = null;
    if (container instanceof MenuBar) {
      item = new MenuBarLabel(initem.name);
      container.addLabel(item);
    } else {
      item = new MenuItem(initem.name,
                          initem.hasOwnProperty('key') ? initem.key : null,
                          initem.action);
      if (initem.hasOwnProperty('enabled') && !initem.enabled) {
        item.setEnabled(false);
      }
      container.addMenuItem(item);
    }
    if (initem.children) {
      let subcontainer = new MenuContainer();
      item.setContainer(subcontainer);
      populate2(initem.children, subcontainer);
    }
  }
}

let shortcutKeyToStr = function(key) {
  const code = key.charCodeAt(0);
  if (code > 64 && code < 91) {  // upper-case
    return "Ctrl+Shift+" + key.toUpperCase();
  } else {  // assume lower-case
    return "Ctrl+" + key.toUpperCase();
  }
};

let globalMenuBar = null;

let initGlobalMenuBar = () => {
  if (globalMenuBar)
    return;
  // create dom elements
  shortCutListener = new ShortCutListener();
  globalMenuBar = new MenuBar(document.getElementById('menubar'));
  populate2(menus, globalMenuBar);
};

document.addEventListener('DOMContentLoaded', initGlobalMenuBar, false);
