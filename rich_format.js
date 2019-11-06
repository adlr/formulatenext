'use strict';

let HTMLWalk = (str, listener) => {
  // TODO: cache these:
  const HTMLText = Module.cwrap('StaticHTMLText', null,
                                ['number', 'string']);
  const HTMLNodeStarted = Module.cwrap('StaticHTMLNodeStarted', null,
                                       ['number', 'string']);
  const HTMLNodeAttribute = Module.cwrap('StaticHTMLNodeAttribute', null,
                                         ['number', 'string', 'string']);
  const HTMLNodeEnded = Module.cwrap('StaticHTMLNodeEnded', null, ['number']);

  // Parse the string
  let template = document.createElement('template');
  template.innerHTML = str;
  let root = template.content.cloneNode(true);  // Not sure we need to cloneNode

  let innerWalk = (list) => {
    for (let i = 0; i < list.length; i++) {
      const node = list[i];
      switch (node.nodeType) {
      case Node.ELEMENT_NODE:
        HTMLNodeStarted(listener, node.tagName);
        if (node.hasAttributes()) {
          for (let j = 0; j < node.attributes.length; j++) {
            HTMLNodeAttribute(listener, node.attributes[j].name,
                              node.attributes[j].value);
          }
        }
        if (node.hasChildNodes()) {
          innerWalk(node.childNodes);
        }
        HTMLNodeEnded(listener);
        break;
      case Node.TEXT_NODE:
        HTMLText(listener, node.textContent);
        break;
      default:
        console.log(`Unable to handle node type ${node.nodeType} in HTMLWalk:`);
        console.log(node);
      }
    }
  };
  innerWalk(root.childNodes);
  console.log('done with inner walk.');
}
