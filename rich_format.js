'use strict';

let g_HTMLText = null;
let g_HTMLNodeStarted = null;
let g_HTMLNodeAttribute = null;
let g_HTMLNodeAttributesEnded = null;
let g_HTMLNodeEnded = null;

let HTMLWalk = (str, listener) => {
  if (g_HTMLText === null) {
    g_HTMLText = Module.cwrap('StaticHTMLText', null,
                              ['number', 'string']);
    g_HTMLNodeStarted = Module.cwrap('StaticHTMLNodeStarted', null,
                                     ['number', 'string']);
    g_HTMLNodeAttribute = Module.cwrap('StaticHTMLNodeAttribute', null,
                                       ['number', 'string',
                                        'string', 'string']);
    g_HTMLNodeAttributesEnded = Module.cwrap('StaticHTMLNodeAttributesEnded',
                                             null, ['number', 'string']);
    g_HTMLNodeEnded = Module.cwrap('StaticHTMLNodeEnded', null, ['number']);
  }

  // Parse the string
  let template = document.createElement('template');
  template.innerHTML = str;
  let root = template.content.cloneNode(true);  // Not sure we need to cloneNode

  let innerWalk = (list) => {
    for (let i = 0; i < list.length; i++) {
      const node = list[i];
      switch (node.nodeType) {
      case Node.ELEMENT_NODE:
        g_HTMLNodeStarted(listener, node.tagName);
        if (node.hasAttributes()) {
          for (let j = 0; j < node.attributes.length; j++) {
            g_HTMLNodeAttribute(listener, node.tagName,
                                node.attributes[j].name,
                                node.attributes[j].value);
          }
        }
        g_HTMLNodeAttributesEnded(listener, node.tagName);
        if (node.hasChildNodes()) {
          innerWalk(node.childNodes);
        }
        g_HTMLNodeEnded(listener);
        break;
      case Node.TEXT_NODE:
        g_HTMLText(listener, node.textContent);
        break;
      default:
        console.log(`Unable to handle node type ${node.nodeType} in HTMLWalk:`);
        console.log(node);
      }
    }
  };
  innerWalk(root.childNodes);
}
